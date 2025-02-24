// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root
    required property Item backgroundItem
    property alias range: glassEffect.range
    property alias blur: glassEffect.blur
    property alias color: glassEffect.color
    property alias backgroundRect: backgroundSourceImage.sourceRect

    ShaderEffectSource {
        anchors.fill: parent
        id: backgroundSourceImage
        sourceRect: Qt.rect(0, 0, width, height)
        sourceItem: targetView
        visible: false
    }


    ShaderEffectSource {
        anchors.fill: parent
        id: noiseImageSource
        sourceRect: Qt.rect(0, 0, width, height)
        sourceItem: noiseImage
        visible: false
    }

    Image {
        anchors.fill: parent
        id: noiseImage
        fillMode: Image.Tile
        horizontalAlignment: Image.AlignLeft
        verticalAlignment: Image.AlignTop
        visible: false
        source: "qrc:/assets/images/noise.png"
    }

    ShaderEffect {
        id: glassEffect
        property variant sourceTex: backgroundSourceImage
        property variant noiseTex: noiseImageSource
        property real range: 0.25;
        property real blur: 0.05;
        property color color: "white"
        anchors.fill: parent
        fragmentShader: "qrc:/assets/shaders/frostedGlass.frag.qsb"
    }
}
