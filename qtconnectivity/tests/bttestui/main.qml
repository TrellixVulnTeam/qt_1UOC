// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import Local 5.2

Rectangle {
    width: 360
    height: 360

    BluetoothDevice {
        id: device
        function evaluateError(error)
        {
            switch (error) {
            case 0: return "Last Error: NoError"
            case 1: return "Last Error: Pairing Error"
            case 100: return "Last Error: Unknown Error"
            case 1000: return "Last Error: <None>"
            }
        }

        function evaluateSocketState(s) {
            switch (s) {
            case 0: return "Socket: Unconnected";
            case 1: return "Socket: HostLookup";
            case 2: return "Socket: Connecting";
            case 3: return "Socket: Connected";
            case 4: return "Socket: Bound";
            case 5: return "Socket: Listening";
            case 6: return "Socket: Closing";
            }
            return "Socket: <None>";
        }


        onErrorOccurred: errorText.text = evaluateError(error)
        onHostModeStateChanged: hostModeText.text = device.hostMode;
        onSocketStateUpdate : socketStateText.text = evaluateSocketState(foobar);
    }

    Text {
        id: errorText
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 3
        text: "Last Error: <None>"
    }
    Text {
        id: hostModeText
        anchors.left: parent.left
        anchors.bottom: socketStateText.top
        anchors.bottomMargin: 3
        text: device.hostMode
    }
    Text {
        id: socketStateText
        anchors.left: parent.left
        anchors.bottom: errorText.top
        anchors.bottomMargin: 3
        text: device.evaluateSocketState(0)
    }
    Text {
        id: secFlagLabel; text: "SecFlags: "
        anchors.left: parent.left
        anchors.bottom: hostModeText.top
        anchors.bottomMargin: 3
    }
    Text {
        anchors.left: secFlagLabel.right
        anchors.bottom: hostModeText.top
        anchors.bottomMargin: 3
        text: device.secFlags
    }

    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 4

        spacing: 8
        Column {
            spacing: 8
            Text{
                width: connectBtn.width
                horizontalAlignment: Text.AlignLeft
                font.pointSize: 12
                wrapMode: Text.WordWrap
                text: "Device Management"
            }
            Button {

                buttonText: "PowerOn"
                onClicked: device.powerOn()
            }
            Button {
                buttonText: "PowerOff"
                onClicked: device.setHostMode(0)
            }
            Button {
                id: connectBtn
                buttonText: "Connectable"
                onClicked: device.setHostMode(1)
            }
            Button {
                buttonText: "Discover"
                onClicked: device.setHostMode(2)
            }
            Button {
                buttonText: "Pair"
                onClicked: device.requestPairingUpdate(true)
            }
            Button {
                buttonText: "Unpair"
                onClicked: device.requestPairingUpdate(false)
            }
            Button {
                buttonText: "Cycle SecFlag"
                onClicked: device.cycleSecurityFlags()
            }
        }
        Column {
            spacing: 8
            Text{
                width: startFullSDiscBtn.width
                horizontalAlignment: Text.AlignLeft
                font.pointSize: 12
                wrapMode: Text.WordWrap
                text: "Device & Service Discovery"
            }
            Button {
                buttonText: "StartDDisc"
                onClicked: device.startDiscovery()
            }
            Button {
                buttonText: "StopDDisc"
                onClicked: device.stopDiscovery()
            }
            Button {
                buttonText: "StartMinSDisc"
                onClicked: device.startServiceDiscovery(true)
            }
            Button {
                id: startFullSDiscBtn
                buttonText: "StartFullSDisc"
                onClicked: device.startServiceDiscovery(false)
            }
            Button {
                id: startRemoteSDiscBtn
                buttonText: "StartRemSDisc"
                onClicked: device.startTargettedServiceDiscovery()
            }
            Button {
                buttonText: "StopSDisc"
                onClicked: device.stopServiceDiscovery();
            }
            Button {
                buttonText: "DumpSDisc"
                onClicked: device.dumpServiceDiscovery();
            }

        }

        Column {
            spacing: 8
            Text{
                width: connectSearchBtn.width
                horizontalAlignment: Text.AlignLeft
                font.pointSize: 12
                wrapMode: Text.WordWrap
                text: "Client & Server Socket"
            }
            Button {
                buttonText: "SocketDump"
                onClicked: device.dumpSocketInformation()
            }
            Button {
                buttonText: "CConnect"
                onClicked: device.connectToService()
            }
            Button {
                id: connectSearchBtn
                buttonText: "ConnectSearch"
                onClicked: device.connectToServiceViaSearch()
            }
            Button {
                buttonText: "CDisconnect"
                onClicked: device.disconnectToService()
            }

            Button {
                buttonText: "CClose"
                onClicked: device.closeSocket()
            }

            Button {
                buttonText: "CAbort"
                onClicked: device.abortSocket()
            }
            Button {
                //Write to all open sockets ABCABC\n
                buttonText: "CSWrite"
                onClicked: device.writeData()
            }
            Button {
                buttonText: "ServerDump"
                onClicked: device.dumpServerInformation()
            }
            Button {
                //Listen on server via port
                buttonText: "SListenPort"
                onClicked: device.serverListenPort()
            }
            Button {
                //Listen on server via uuid
                buttonText: "SListenUuid"
                onClicked: device.serverListenUuid()
            }
            Button {
                //Close Bluetooth server
                buttonText: "SClose"
                onClicked: device.serverClose()
            }
        }
        Column {
            spacing: 8
            Text{
                width: resetBtn.width
                horizontalAlignment: Text.AlignLeft
                font.pointSize: 12
                wrapMode: Text.WordWrap
                text: "Misc Controls"
            }
            Button {
                buttonText: "Dump"
                onClicked: device.dumpInformation()
            }
            Button {
                id: resetBtn
                buttonText: "Reset"
                onClicked: device.reset()
            }
        }
    }
}
