import QtQuick 2.0
import QtQuick.Controls 1.1

Rectangle {
    width: 250
    height: 600
    Text {
        id: name
        text: qsTr("Hello from QML!!!")
    }
    SpinBox {
        id: minValueSpin
        decimals: 2
        minimumValue: -10000
        maximumValue: 1000000
        width: 80
        value: rulercolors.minValue
        onValueChanged: rulercolors.minValue = value
    }
    SpinBox {
        id: maxValueSpin
        decimals: 2
        width: 80
        minimumValue: -10000
        maximumValue: 1000000
        value: rulercolors.maxValue
        anchors.left: minValueSpin.right
        anchors.leftMargin: 10
        onValueChanged: rulercolors.maxValue = value
    }

   /* Rectangle
        id: rectangle
        color: "#978c8c"
        width: 200
        height: 200
        anchors.top: name.top

        Text {
            id: text
            text: "This is QML code.\n(Click to pause)"
            font.pointSize: 14
            anchors.centerIn: parent
            PropertyAnimation {
                id: animation
                target: text
                property: "rotation"
                from: 0; to: 360; duration: 5000
                loops: Animation.Infinite
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: if (animation.paused) { animation.resume();text.text = text.text.concat("!") } else animation.pause()
        }
        Component.onCompleted: animation.start()
    }*/
    Column {
        id: colorRamp
        anchors.topMargin: 10
        y: 60
        Repeater {
            //model: ["yellow", "red", "green", "darkgrey", "blue","yellow", "red", "green", "darkgrey", "blue", "darkgrey", "blue"]
            model: rulercolors.colors
            Rectangle {
                width: 60; height: 150 / rulercolors.count
                color: modelData
            }
        }
    }
    Text {
        id: maxValue
        text: rulercolors.labels[4]
        anchors.left: colorRamp.right
        anchors.top: colorRamp.top
        anchors.topMargin: -height/2
        anchors.leftMargin: 5
    }
    Text {
        id: upperQuartileValue
        text: rulercolors.labels[3]
        anchors.left: colorRamp.right
        anchors.top:  colorRamp.top
        anchors.topMargin: colorRamp.height/4 - height/2
        anchors.leftMargin: 5
        visible: colorRamp.height>100
    }
    Text {
        id: centerValue
        text: rulercolors.labels[2]
        anchors.left: colorRamp.right
        anchors.verticalCenter:  colorRamp.verticalCenter
        anchors.topMargin: height/2
        anchors.leftMargin: 5
    }
    Text {
        id: lowerQuartileValue
        text: rulercolors.labels[1]
        anchors.left: colorRamp.right
        anchors.top:  colorRamp.top
        anchors.topMargin: colorRamp.height*3/4 - height/2
        anchors.leftMargin: 5
        visible: colorRamp.height>100
    }
    Text {
        id: minValue
        text: rulercolors.labels[0]
        anchors.left: colorRamp.right
        anchors.bottom: colorRamp.bottom
        anchors.topMargin: height/2
        anchors.leftMargin: 5
    }
}
