// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//Own
#include "graphicsscene.h"
#include "states.h"
#include "boat.h"
#include "submarine.h"
#include "torpedo.h"
#include "bomb.h"
#include "animationmanager.h"
#include "qanimationstate.h"
#include "progressitem.h"
#include "textinformationitem.h"

//Qt
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QFinalState>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QStateMachine>
#include <QXmlStreamReader>

GraphicsScene::GraphicsScene(int x, int y, int width, int height, Mode mode, QObject *parent)
    : QGraphicsScene(x, y, width, height, parent), mode(mode), boat(new Boat)
{
//![2]
    PixmapItem *backgroundItem = new PixmapItem(QStringLiteral("background"), mode);
    backgroundItem->setZValue(1);
    backgroundItem->setPos(0,0);
    addItem(backgroundItem);

    PixmapItem *surfaceItem = new PixmapItem(QStringLiteral("surface"), mode);
    surfaceItem->setZValue(3);
    surfaceItem->setPos(0, sealLevel() - surfaceItem->boundingRect().height() / 2);
    addItem(surfaceItem);

    //The item that displays score and level
    progressItem = new ProgressItem(backgroundItem);

    textInformationItem = new TextInformationItem(backgroundItem);

    textInformationItem->setMessage(QString("Select new game from the menu or press Ctrl+N to start!<br/>Press left or right to move the ship and up to drop bombs."), false);
    textInformationItem->setPos(backgroundItem->boundingRect().center().x() - textInformationItem->boundingRect().size().width() / 2,
                                backgroundItem->boundingRect().height() * 3 / 4);

    //We create the boat
    addItem(boat);
    boat->setPos(this->width()/2, sealLevel() - boat->size().height());
    boat->hide();
//![2]

    //Parse the xml that contains all data of the game
    QXmlStreamReader reader;
    QFile file(":data.xml");
    file.open(QIODevice::ReadOnly);
    reader.setDevice(&file);
    LevelDescription currentLevel;
    while (!reader.atEnd()) {
         reader.readNext();
         if (reader.tokenType() == QXmlStreamReader::StartElement) {
             if (reader.name() == u"submarine") {
                 SubmarineDescription desc;
                 desc.name = reader.attributes().value("name").toString();
                 desc.points = reader.attributes().value("points").toInt();
                 desc.type = reader.attributes().value("type").toInt();
                 submarinesData.append(desc);
             } else if (reader.name() == u"level") {
                 currentLevel.id = reader.attributes().value("id").toInt();
                 currentLevel.name = reader.attributes().value("name").toString();
             } else if (reader.name() == u"subinstance") {
                 currentLevel.submarines.append(qMakePair(reader.attributes().value("type").toInt(),
                                                          reader.attributes().value("nb").toInt()));
             }
         } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            if (reader.name() == u"level") {
                levelsData.insert(currentLevel.id, currentLevel);
                currentLevel.submarines.clear();
            }
         }
   }
}

qreal GraphicsScene::sealLevel() const
{
    return (mode == Big) ? 220 : 160;
}

void GraphicsScene::setupScene(QAction *newAction, QAction *quitAction)
{
    static constexpr int nLetters = 10;
    static constexpr struct {
        char const *pix;
        qreal initX, initY;
        qreal destX, destY;
    } logoData[nLetters] = {
        {"s",   -1000, -1000, 300, 150 },
        {"u",    -800, -1000, 350, 150 },
        {"b",    -600, -1000, 400, 120 },
        {"dash", -400, -1000, 460, 150 },
        {"a",    1000,  2000, 350, 250 },
        {"t",     800,  2000, 400, 250 },
        {"t2",    600,  2000, 430, 250 },
        {"a2",    400,  2000, 465, 250 },
        {"q",     200,  2000, 510, 250 },
        {"excl",    0,  2000, 570, 220 } };

    QSequentialAnimationGroup *lettersGroupMoving = new QSequentialAnimationGroup(this);
    QParallelAnimationGroup *lettersGroupFading = new QParallelAnimationGroup(this);

    for (int i = 0; i < nLetters; ++i) {
        PixmapItem *logo = new PixmapItem(QLatin1String(":/logo-") + logoData[i].pix, this);
        logo->setPos(logoData[i].initX, logoData[i].initY);
        logo->setZValue(i + 3);
        //creation of the animations for moving letters
        QPropertyAnimation *moveAnim = new QPropertyAnimation(logo, "pos", lettersGroupMoving);
        moveAnim->setEndValue(QPointF(logoData[i].destX, logoData[i].destY));
        moveAnim->setDuration(200);
        moveAnim->setEasingCurve(QEasingCurve::OutElastic);
        lettersGroupMoving->addPause(50);
        //creation of the animations for fading out the letters
        QPropertyAnimation *fadeAnim = new QPropertyAnimation(logo, "opacity", lettersGroupFading);
        fadeAnim->setDuration(800);
        fadeAnim->setEndValue(0);
        fadeAnim->setEasingCurve(QEasingCurve::OutQuad);
    }

//![3]
    QStateMachine *machine = new QStateMachine(this);

    //This state is when the player is playing
    PlayState *gameState = new PlayState(this, machine);

    //Final state
    QFinalState *finalState = new QFinalState(machine);

    //Animation when the player enters the game
    QAnimationState *lettersMovingState = new QAnimationState(machine);
    lettersMovingState->setAnimation(lettersGroupMoving);

    //Animation when the welcome screen disappears
    QAnimationState *lettersFadingState = new QAnimationState(machine);
    lettersFadingState->setAnimation(lettersGroupFading);

    //if it is a new game then we fade out the welcome screen and start playing
    lettersMovingState->addTransition(newAction, &QAction::triggered, lettersFadingState);
    lettersFadingState->addTransition(lettersFadingState, &QAnimationState::animationFinished, gameState);

    //New Game is triggered then player starts playing
    gameState->addTransition(newAction, &QAction::triggered, gameState);

    //Wanna quit, then connect to CTRL+Q
    gameState->addTransition(quitAction, &QAction::triggered, finalState);
    lettersMovingState->addTransition(quitAction, &QAction::triggered, finalState);

    //Welcome screen is the initial state
    machine->setInitialState(lettersMovingState);

    machine->start();

    //We reach the final state, then we quit
    connect(machine, &QStateMachine::finished, qApp, &QApplication::quit);
//![3]
}

void GraphicsScene::addItem(Bomb *bomb)
{
    bombs.insert(bomb);
    connect(bomb, &Bomb::bombExecutionFinished,
            this, &GraphicsScene::onBombExecutionFinished);
    QGraphicsScene::addItem(bomb);
}

void GraphicsScene::addItem(Torpedo *torpedo)
{
    torpedos.insert(torpedo);
    connect(torpedo, &Torpedo::torpedoExecutionFinished,
            this, &GraphicsScene::onTorpedoExecutionFinished);
    QGraphicsScene::addItem(torpedo);
}

void GraphicsScene::addItem(SubMarine *submarine)
{
    submarines.insert(submarine);
    connect(submarine, &SubMarine::subMarineExecutionFinished,
            this, &GraphicsScene::onSubMarineExecutionFinished);
    QGraphicsScene::addItem(submarine);
}

void GraphicsScene::addItem(QGraphicsItem *item)
{
    QGraphicsScene::addItem(item);
}

void GraphicsScene::onBombExecutionFinished()
{
    Bomb *bomb = qobject_cast<Bomb *>(sender());
    if (!bomb)
        return;
    bombs.remove(bomb);
    bomb->deleteLater();
    boat->setBombsLaunched(boat->bombsLaunched() - 1);
}

void GraphicsScene::onTorpedoExecutionFinished()
{
    Torpedo *torpedo = qobject_cast<Torpedo *>(sender());
    if (!torpedo)
        return;
    torpedos.remove(torpedo);
    torpedo->deleteLater();
}

void GraphicsScene::onSubMarineExecutionFinished()
{
    SubMarine *submarine = qobject_cast<SubMarine *>(sender());
    if (!submarine)
        return;
    submarines.remove(submarine);
    if (submarines.count() == 0)
        emit allSubMarineDestroyed(submarine->points());
    else
        emit subMarineDestroyed(submarine->points());
    submarine->deleteLater();
}

void GraphicsScene::clearScene()
{
    for (SubMarine *sub : qAsConst(submarines)) {
        // make sure to not go into onSubMarineExecutionFinished
        sub->disconnect(this);
        sub->destroy();
        sub->deleteLater();
    }

    for (Torpedo *torpedo : qAsConst(torpedos)) {
        // make sure to not go into onTorpedoExecutionFinished
        torpedo->disconnect(this);
        torpedo->destroy();
        torpedo->deleteLater();
    }

    for (Bomb *bomb : qAsConst(bombs)) {
        // make sure to not go into onBombExecutionFinished
        bomb->disconnect(this);
        bomb->destroy();
        bomb->deleteLater();
    }

    submarines.clear();
    bombs.clear();
    torpedos.clear();

    AnimationManager::self()->unregisterAllAnimations();

    boat->stop();
    boat->hide();
    boat->setEnabled(true);
}
