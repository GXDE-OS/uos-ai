import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

import org.deepin.dtk 1.0
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

PanelPopup {
    property int originalRadius: 0
    Control {
        id:tips
        visible: false
        width: 208
        height: 30

        RowLayout {
            spacing: 6
            anchors.fill: parent
            Item {
                width: 2
                height: 1
            }

            Image {
                width: 16
                height: 16
                sourceSize.width: 16
                sourceSize.height: 16
                source: "qrc:/icons/deepin/builtin/icons/UosAiAssistant.svg"
            }

           Label {
               rightPadding: 10
               verticalAlignment: Text.AlignVCenter // 垂直居中对齐文本
               Layout.fillHeight: true
               Layout.fillWidth: true
               font: DTK.fontManager.t8
               elide: Text.ElideRight
               text: qsTr("Drag the document here and I'll take care of it for you")
           }
        }

        DropArea {
            anchors.fill: parent
            onEntered: function(drag) {
                if (docToast.visiable || drag.urls.length !== 1 ||
                        !Applet.isSupportDrop(drag.urls[0].toString())) {
                    drag.accepted = false
                    return false
                } else {
                    console.info("Valid document drag detected, switching to doc view")
                    switchToDoc()
                    Applet.onShowDocArea()
                    return true
                }
            }
        }
    }

    Control {
        id: docArea
        visible: false
        width: 360
        height: 104
        property bool isInDocDropArea : false
        Timer {
            id: closeTimer
            interval: 0 // 设置定时器的间隔时间为 0 毫秒
            onTriggered: {
                docToast.close()
            }
            repeat: false // 设置定时器为非重复模式
            running: true // 启动定时器
        }
        Timer {
            id: switchToTipsTimer
            interval: 0 // 设置定时器的间隔时间为 0 毫秒
            onTriggered: {
                switchToTips()
            }
            repeat: false // 设置定时器为非重复模式
            running: true // 启动定时器
        }
        DropArea {
            anchors.fill: parent
            onDropped: {
                console.debug("Document dropped, closing toast")
                closeTimer.start()
            }
            onExited: {
                if(!docArea.isInDocDropArea) {
                    console.debug("Drag exited doc area, scheduling switch to tips")
                    switchToTipsTimer.interval = 2000
                    switchToTipsTimer.start()
                }
            }
            onEntered: {
                if (switchToTipsTimer.running) {
                    console.debug("Drag re-entered doc area, canceling tips switch")
                    switchToTipsTimer.stop()
                }
            }
        }
        ColumnLayout{
            spacing: 7
            anchors.centerIn : parent
            Layout.fillHeight : true
            width: parent.width

            RowLayout {
                spacing: 8
                height: 16
                width: parent.width
                Layout.leftMargin : 6

                Image {
                    width: 16
                    height: 16
                    sourceSize.width: 16
                    sourceSize.height: 16
                    source: "qrc:/icons/deepin/builtin/icons/UosAiAssistant.svg"
                }

                Label {
                    id: title
                    rightPadding: 10
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    property string defText :  qsTr("Drag the document here and I'll take care of it for you")
                    font: DTK.fontManager.t8
                    text: defText
                    verticalAlignment: Text.AlignVCenter // 垂直居中对齐文本
                }
            }

            GridView {
                Layout.fillWidth : true
                Layout.leftMargin : 6
                model: itemModel
                height: cellHeight + 10
                width: cellWidth * 4
                cellWidth: 80 + 8
                cellHeight: 64

                delegate: DocDroparea {
                    text: model.text
                    desc: model.desc
                    titleControl: title
                    imageSource: model.imageFile
                    onDroped : function (url){
                        Applet.docAction(index, url)
                        closeTimer.start()
                    }
                }
            }
        }
    }

    function switchToTips() {
        console.debug("Switching to tips view")
        tips.visible = true
        docArea.visible = false
        this.width = tips.width
        this.height = tips.height
        this.DockPanelPositioner.bounding = Qt.rect(iconPoint.x + width / 2 * ((Panel.position + 1) % 2),
                                                    iconPoint.y + height / 2 * (Panel.position % 2),
                                                    width, height)
    }

    function switchToDoc() {
        console.debug("Switching to doc view")
        tips.visible = false
        docArea.visible = true
        this.width = docArea.width
        this.height = docArea.height
        this.DockPanelPositioner.bounding = Qt.rect(iconPoint.x + width / 2 * ((Panel.position + 1) % 2),
                                                    iconPoint.y + height / 2 * (Panel.position % 2),
                                                    width, height)
    }

    // 修改圆角
//    onPopupVisibleChanged: {
//        if (!popupWindow)
//            return
//        if (popupVisible) {
//            originalRadius = popupWindow.D.DWindow.windowRadius
//            popupWindow.D.DWindow.windowRadius = 6
//        } else if (originalRadius > 0) {
//            popupWindow.D.DWindow.windowRadius = originalRadius
//        }
//    }
}
