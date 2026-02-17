import QtQuick 2.15
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5

import org.deepin.dtk 1.0 as D
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

Rectangle {
    id: docDropArea
    width: 82
    height: 66
    color: D.DTK.themeType == 1 ? Qt.rgba(255,255,255,0.4) : Qt.rgba(0,0,0,0.3)
    radius: 5
    border.color: D.DTK.themeType == 1 ? Qt.rgba(0,0,0,0.03) : Qt.rgba(0,0,0,0.45)
    border.width: 1
    property string text: ""
    property string desc: ""
    property var nowThemeType: D.DTK.themeType
    property var titleControl
    property var imageSource

    onNowThemeTypeChanged: {
        docDropArea.color = D.DTK.themeType == 1 ? Qt.rgba(255,255,255,0.4) : Qt.rgba(0,0,0,0.3)
    }

    signal droped(string url)

    // 启用抗锯齿和高质量渲染
    layer.enabled: true
    layer.smooth: true
    layer.samples: 4

    Rectangle {
        id: docInnerShadowArea
        anchors.centerIn: parent
        width: parent.width - 2
        height: parent.height - 2
        color: "transparent"
        radius: 4
        ColumnLayout{
            spacing: 3
            width: parent.width
            anchors.centerIn: parent
            D.Control {
                id: docDropIconControl
                anchors.horizontalCenter: parent.horizontalCenter
                width: 24
                height: 24
                D.DciIcon {
                    name: imageSource
                    anchors.centerIn: parent
                    palette: D.DTK.makeIconPalette(docDropIconControl.palette)
                    mode: D.ColorSelector.controlState
                    theme: D.DTK.themeType
                    sourceSize: Qt.size(24, 24)
                }
            }

            Label {
                horizontalAlignment: Qt.AlignHCenter//文本居中
                Layout.fillHeight: true
                Layout.fillWidth: true
                elide: Text.ElideRight
                font: D.DTK.fontManager.t10
                text: docDropArea.text
                // 启用文字抗锯齿
                renderType: Text.NativeRendering
                antialiasing: true
            }

        }
    }

    DropArea {
        anchors.fill: parent
        onEntered: function(drag) {
            console.debug("Drag entered drop area")
            if (switchToTipsTimer.running) {
                switchToTipsTimer.stop()
            }
            docArea.isInDocDropArea = true
            if (titleControl)
                titleControl.text = desc
            docScaleAnimation.stop()
            docScaleAnimation.to =1.05
            docScaleAnimation.start()
            docDropArea.color = D.DTK.themeType == 1 ? Qt.rgba(255,255,255,0.6) : Qt.rgba(0,0,0,0.4)
        }

        onExited: function() {
            console.debug("Drag exited drop area")
            docArea.isInDocDropArea = false
            if (titleControl)
                titleControl.text = titleControl.defText
            docScaleAnimation.stop()
            docScaleAnimation.to =1.0
            docScaleAnimation.start()
            docDropArea.color = D.DTK.themeType == 1 ? Qt.rgba(255,255,255,0.4) : Qt.rgba(0,0,0,0.3)
        }

        onDropped: function(drop) {
            var urls = drop.urls
            if (urls.length !== 1) {
                console.warn(`Invalid drop - expected 1 URL, got ${urls.length}`)
                return
            }

            console.info(`File dropped: ${urls[0].toString()}`)
            droped(urls[0].toString())
            docScaleAnimation.stop()
            docScaleAnimation.to =1.0
            docScaleAnimation.start()
            docDropArea.color = D.DTK.themeType == 1 ? Qt.rgba(255,255,255,0.4) : Qt.rgba(0,0,0,0.3)
        }

        NumberAnimation {
            id : docScaleAnimation
            target: docDropArea
            property : "scale"
            duration : 200
            to : 1.05
            easing.type: Easing.OutCubic
        }

    }
}
