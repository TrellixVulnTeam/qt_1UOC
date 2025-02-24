import QtQuick
Item {
    id: root

    ComponentType { // normal type here
        id: normal
        property string text: "indirect component"
    }

    Component {
        id: accessibleNormal
        ComponentType {
            id: inaccessibleNormal
        }
    }

    property Component p2: ComponentType { id: accessible; property string text: "foo" }
    property Component p3: Rectangle { id: inaccessible; property string text: "bar" }

    TableView {
        delegate: Rectangle { id: inaccessibleDelegate }
    }

    TableView {
        delegate: ComponentType { id: accessibleDelegate }
    }

    property alias accessibleNormalProgress: accessibleNormal.progress
    property alias accessibleNormalUrl: accessibleNormal.url
    property url urlClone: root.accessibleNormalUrl

    property alias delegateUrlClone: accessibleDelegate.url
}
