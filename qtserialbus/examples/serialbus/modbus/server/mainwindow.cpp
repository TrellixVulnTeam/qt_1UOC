// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "settingsdialog.h"
#include "ui_mainwindow.h"

#include <QModbusRtuSerialServer>
#include <QModbusTcpServer>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStatusBar>
#include <QUrl>

enum ModbusConnection {
    Serial,
    Tcp
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupWidgetContainers();

#if QT_CONFIG(modbus_serialport)
    ui->connectType->setCurrentIndex(0);
    onCurrentConnectTypeChanged(0);
#else
    // lock out the serial port option
    ui->connectType->setCurrentIndex(1);
    onCurrentConnectTypeChanged(1);
    ui->connectType->setEnabled(false);
#endif

    m_settingsDialog = new SettingsDialog(this);
    initActions();
}

MainWindow::~MainWindow()
{
    if (modbusDevice)
        modbusDevice->disconnectDevice();
    delete modbusDevice;

    delete ui;
}

void MainWindow::initActions()
{
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionExit->setEnabled(true);
    ui->actionOptions->setEnabled(true);

    connect(ui->connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectButtonClicked);
    connect(ui->actionConnect, &QAction::triggered,
            this, &MainWindow::onConnectButtonClicked);
    connect(ui->actionDisconnect, &QAction::triggered,
            this, &MainWindow::onConnectButtonClicked);
    connect(ui->connectType, &QComboBox::currentIndexChanged,
            this, &MainWindow::onCurrentConnectTypeChanged);

    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionOptions, &QAction::triggered, m_settingsDialog, &QDialog::show);
}

void MainWindow::onCurrentConnectTypeChanged(int index)
{
    if (modbusDevice) {
        modbusDevice->disconnect();
        delete modbusDevice;
        modbusDevice = nullptr;
    }

    auto type = static_cast<ModbusConnection>(index);
    if (type == Serial) {
#if QT_CONFIG(modbus_serialport)
        modbusDevice = new QModbusRtuSerialServer(this);
#endif
    } else if (type == Tcp) {
        modbusDevice = new QModbusTcpServer(this);
        if (ui->portEdit->text().isEmpty())
            ui->portEdit->setText(QLatin1String("127.0.0.1:502"));
    }
    ui->listenOnlyBox->setEnabled(type == Serial);

    if (!modbusDevice) {
        ui->connectButton->setDisabled(true);
        statusBar()->showMessage(tr("Could not create Modbus server."), 5000);
    } else {
        QModbusDataUnitMap reg;
        reg.insert(QModbusDataUnit::Coils, { QModbusDataUnit::Coils, 0, 10 });
        reg.insert(QModbusDataUnit::DiscreteInputs, { QModbusDataUnit::DiscreteInputs, 0, 10 });
        reg.insert(QModbusDataUnit::InputRegisters, { QModbusDataUnit::InputRegisters, 0, 10 });
        reg.insert(QModbusDataUnit::HoldingRegisters, { QModbusDataUnit::HoldingRegisters, 0, 10 });

        modbusDevice->setMap(reg);

        connect(modbusDevice, &QModbusServer::dataWritten,
                this, &MainWindow::updateWidgets);
        connect(modbusDevice, &QModbusServer::stateChanged,
                this, &MainWindow::onStateChanged);
        connect(modbusDevice, &QModbusServer::errorOccurred,
                this, &MainWindow::handleDeviceError);

        connect(ui->listenOnlyBox, &QCheckBox::toggled, this, [this](bool toggled) {
            if (modbusDevice)
                modbusDevice->setValue(QModbusServer::ListenOnlyMode, toggled);
        });
        emit ui->listenOnlyBox->toggled(ui->listenOnlyBox->isChecked());
        connect(ui->setBusyBox, &QCheckBox::toggled, this, [this](bool toggled) {
            if (modbusDevice)
                modbusDevice->setValue(QModbusServer::DeviceBusy, toggled ? 0xffff : 0x0000);
        });
        emit ui->setBusyBox->toggled(ui->setBusyBox->isChecked());

        setupDeviceData();
    }
}

void MainWindow::handleDeviceError(QModbusDevice::Error newError)
{
    if (newError == QModbusDevice::NoError || !modbusDevice)
        return;

    statusBar()->showMessage(modbusDevice->errorString(), 5000);
}

void MainWindow::onConnectButtonClicked()
{
    bool intendToConnect = (modbusDevice->state() == QModbusDevice::UnconnectedState);

    statusBar()->clearMessage();

    if (intendToConnect) {
        if (static_cast<ModbusConnection>(ui->connectType->currentIndex()) == Serial) {
            modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
                ui->portEdit->text());
#if QT_CONFIG(modbus_serialport)
            modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter,
                m_settingsDialog->settings().parity);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
                m_settingsDialog->settings().baud);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
                m_settingsDialog->settings().dataBits);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
                m_settingsDialog->settings().stopBits);
#endif
        } else {
            const QUrl url = QUrl::fromUserInput(ui->portEdit->text());
            modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, url.port());
            modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, url.host());
        }
        modbusDevice->setServerAddress(ui->serverEdit->text().toInt());
        if (!modbusDevice->connectDevice()) {
            statusBar()->showMessage(tr("Connect failed: ") + modbusDevice->errorString(), 5000);
        } else {
            ui->actionConnect->setEnabled(false);
            ui->actionDisconnect->setEnabled(true);
        }
    } else {
        modbusDevice->disconnectDevice();
        ui->actionConnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(false);
    }
}

void MainWindow::onStateChanged(int state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);
    ui->actionConnect->setEnabled(!connected);
    ui->actionDisconnect->setEnabled(connected);

    if (state == QModbusDevice::UnconnectedState)
        ui->connectButton->setText(tr("Connect"));
    else if (state == QModbusDevice::ConnectedState)
        ui->connectButton->setText(tr("Disconnect"));
}

void MainWindow::coilChanged(int id)
{
    QAbstractButton *button = coilButtons.button(id);
    bitChanged(id, QModbusDataUnit::Coils, button->isChecked());
}

void MainWindow::discreteInputChanged(int id)
{
    QAbstractButton *button = discreteButtons.button(id);
    bitChanged(id, QModbusDataUnit::DiscreteInputs, button->isChecked());
}

void MainWindow::bitChanged(int id, QModbusDataUnit::RegisterType table, bool value)
{
    if (!modbusDevice)
        return;

    if (!modbusDevice->setData(table, quint16(id), value))
        statusBar()->showMessage(tr("Could not set data: ") + modbusDevice->errorString(), 5000);
}

void MainWindow::setRegister(const QString &value)
{
    if (!modbusDevice)
        return;

    const QString objectName = QObject::sender()->objectName();
    if (registers.contains(objectName)) {
        bool ok = true;
        const quint16 id = quint16(QObject::sender()->property("ID").toUInt());
        if (objectName.startsWith(QStringLiteral("inReg")))
            ok = modbusDevice->setData(QModbusDataUnit::InputRegisters, id, value.toUShort(&ok, 16));
        else if (objectName.startsWith(QStringLiteral("holdReg")))
            ok = modbusDevice->setData(QModbusDataUnit::HoldingRegisters, id, value.toUShort(&ok, 16));

        if (!ok)
            statusBar()->showMessage(tr("Could not set register: ") + modbusDevice->errorString(),
                                     5000);
    }
}

void MainWindow::updateWidgets(QModbusDataUnit::RegisterType table, int address, int size)
{
    for (int i = 0; i < size; ++i) {
        quint16 value;
        QString text;
        switch (table) {
        case QModbusDataUnit::Coils:
            modbusDevice->data(QModbusDataUnit::Coils, quint16(address + i), &value);
            coilButtons.button(address + i)->setChecked(value);
            break;
        case QModbusDataUnit::HoldingRegisters:
            modbusDevice->data(QModbusDataUnit::HoldingRegisters, quint16(address + i), &value);
            registers.value(QStringLiteral("holdReg_%1").arg(address + i))->setText(text
                .setNum(value, 16));
            break;
        default:
            break;
        }
    }
}

// -- private

void MainWindow::setupDeviceData()
{
    if (!modbusDevice)
        return;

    for (quint16 i = 0; i < coilButtons.buttons().count(); ++i)
        modbusDevice->setData(QModbusDataUnit::Coils, i, coilButtons.button(i)->isChecked());

    for (quint16 i = 0; i < discreteButtons.buttons().count(); ++i) {
        modbusDevice->setData(QModbusDataUnit::DiscreteInputs, i,
            discreteButtons.button(i)->isChecked());
    }

    bool ok;
    for (QLineEdit *widget : qAsConst(registers)) {
        if (widget->objectName().startsWith(QStringLiteral("inReg"))) {
            modbusDevice->setData(QModbusDataUnit::InputRegisters, quint16(widget->property("ID").toUInt()),
                widget->text().toUShort(&ok, 16));
        } else if (widget->objectName().startsWith(QStringLiteral("holdReg"))) {
            modbusDevice->setData(QModbusDataUnit::HoldingRegisters, quint16(widget->property("ID").toUInt()),
                widget->text().toUShort(&ok, 16));
        }
    }
}

void MainWindow::setupWidgetContainers()
{
    coilButtons.setExclusive(false);
    discreteButtons.setExclusive(false);

    QRegularExpression regexp(QStringLiteral("coils_(?<ID>\\d+)"));
    const QList<QCheckBox *> coils = findChildren<QCheckBox *>(regexp);
    for (QCheckBox *cbx : coils)
        coilButtons.addButton(cbx, regexp.match(cbx->objectName()).captured("ID").toInt());
    connect(&coilButtons, &QButtonGroup::idClicked, this, &MainWindow::coilChanged);

    regexp.setPattern(QStringLiteral("disc_(?<ID>\\d+)"));
    const QList<QCheckBox *> discs = findChildren<QCheckBox *>(regexp);
    for (QCheckBox *cbx : discs)
        discreteButtons.addButton(cbx, regexp.match(cbx->objectName()).captured("ID").toInt());
    connect(&discreteButtons, &QButtonGroup::idClicked, this, &MainWindow::discreteInputChanged);

    regexp.setPattern(QLatin1String("(in|hold)Reg_(?<ID>\\d+)"));
    const QList<QLineEdit *> qle = findChildren<QLineEdit *>(regexp);
    for (QLineEdit *lineEdit : qle) {
        registers.insert(lineEdit->objectName(), lineEdit);
        lineEdit->setProperty("ID", regexp.match(lineEdit->objectName()).captured("ID").toInt());
        lineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[0-9a-f]{0,4}"),
            QRegularExpression::CaseInsensitiveOption), this));
        connect(lineEdit, &QLineEdit::textChanged, this, &MainWindow::setRegister);
    }
}
