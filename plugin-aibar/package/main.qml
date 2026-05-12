// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import QtQuick.Window 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

AppletItem {
    id: aibar
    property bool shouldVisible: Applet.visible
    property int dockSize: Panel.rootObject.dockSize
    property int dockOrder: 26
    property bool useColumnLayout: Panel.position % 2
    property ListModel itemModel: ListModel{}
    property int iconCount: 1
    property bool meetingIconVisible: true
    property var meetAssistantStatus: Applet.getNowMeetAssistantStatus()
    property point iconPoint: Qt.point(0, 0)
    property var iconWidth : Panel.rootObject.dockItemMaxSize * 2 / 3 > 36 ? 36 : Panel.rootObject.dockItemMaxSize * 2 / 3
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : iconWidth * iconCount + 10
    implicitHeight: useColumnLayout ? iconWidth * iconCount + 10 : Panel.rootObject.dockSize
    PanelToolTip {
        id: toolTip
        text: qsTr("UOS AI Bar")
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: {
            if (mouse.button == Qt.LeftButton) {
                mouse.accepted = false
            }
            if (mouse.button == Qt.RightButton) {
                mouse.accepted = true
            }
        }

        Loader {
            anchors.fill: parent
            //判断当前任务栏位置，加载不同component
            sourceComponent: useColumnLayout ? verticalLayoutView : horizontalLayoutView
        }
    }

    MeetingToast {
        id: meetingToast
        popupX: DockPanelPositioner.x
        popupY: DockPanelPositioner.y
    }

    DocumentToast {
        id: docToast
        popupX: DockPanelPositioner.x
        popupY: DockPanelPositioner.y
    }

    Component {
        id: horizontalLayoutView
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            RowLayout{
                anchors.centerIn: parent
                spacing: 0

                D.DciIcon {
                    id: aiBarIcon1
                    name: "UosAiAssistant"
                    sourceSize: Qt.size(iconWidth , iconWidth)
                    Layout.rightMargin: 10
                    retainWhileLoading: true

                    // DropArea {
                    //     anchors.fill: parent
                    //     onDropped: function(drop) {
                    //         if (docToast.visible)
                    //             return;

                    //         var urls = drop.urls
                    //         if (urls.length !== 1) {
                    //             return
                    //         }
                    //         Applet.handleDrop(urls[0].toString());
                    //     }

                    //     onEntered: function(drag) {
                    //         if (docToast.popupVisible || drag.urls.length !== 1 ||
                    //                 !Applet.isSupportDrop(drag.urls[0].toString())) {
                    //             drag.accepted = false
                    //             return false;
                    //         }
                    //         // for test
                    //         //dragCon.onDragActivated(drag.urls)
                    //     }
                    // }

                    Connections {
                           target: Panel.rootObject
                           function onDockCenterPartPosChanged()
                           {
                               iconPoint = aiBarIcon1.mapToItem(null, 0, 0)
                           }
                    }
                    Component.onCompleted : {
                        iconPoint = aiBarIcon1.mapToItem(null, 0, 0)
                    }
                }
                D.DciIcon {
                    id: aiMeetingIcon1
                    visible: meetingIconVisible
                    name: "UosAiAssistant"
                    sourceSize: Qt.size(iconWidth , iconWidth)
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            console.info("Opening meeting assistant")
                            
                            Applet.onClickRecommend()
                        }
                    }
                }
            }
            Timer {
                id: toolTipShowTimer2
                interval: 50
                onTriggered: {
                    var point = Applet.rootObject.mapToItem(null, Applet.rootObject.width / 2, Applet.rootObject.height / 2)
                    toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
                    toolTip.open()
                }
            }

            HoverHandler {
                onHoveredChanged: {
                    if (hovered) {
                        toolTipShowTimer2.start()
                    } else {
                        if (toolTipShowTimer2.running) {
                            toolTipShowTimer2.stop()
                        }

                        toolTip.close()
                    }
                }
            }

            MouseArea {
                anchors.fill : parent
                hoverEnabled : true
                preventStealing : true
                onClicked : {
                    Applet.onClickIcon()
                }
            }
        }
    }

    Component {
        id: verticalLayoutView
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 0

                D.DciIcon {
                    id: aiBarIcon2
                    name: "UosAiAssistant"
                    sourceSize: Qt.size(iconWidth , iconWidth)
                    Layout.bottomMargin: 10

                    // DropArea {
                    //     anchors.fill: parent
                    //     onDropped: function(drop) {
                    //         if (docToast.visible) {
                    //             console.info("Drop ignored - doc toast already visible")
                    //             return;
                    //         }

                    //         var urls = drop.urls
                    //         if (urls.length !== 1) {
                    //             console.warn(`Invalid drop - expected 1 URL, got ${urls.length}`)
                    //             return
                    //         }
                    //         console.info(`Processing dropped file: ${urls[0].toString()}`)
                    //         Applet.handleDrop(urls[0].toString());
                    //     }

                    //     onEntered: function(drag) {
                    //         if (docToast.popupVisible || drag.urls.length !== 1 ||
                    //                 !Applet.isSupportDrop(drag.urls[0].toString())) {
                    //             drag.accepted = false
                    //             return false;
                    //         }
                    //         // for test
                    //         //dragCon.onDragActivated(drag.urls)
                    //     }
                    // }
                    Connections {
                        target: Panel.rootObject
                        function onDockCenterPartPosChanged()
                        {
                            iconPoint = aiBarIcon2.mapToItem(null, 0, 0)
                        }
                    }
                    Component.onCompleted : {
                        iconPoint = aiBarIcon2.mapToItem(null, 0, 0)
                    }
                }
                D.DciIcon {
                    id: aiMeetingIcon2
                    visible: meetingIconVisible
                    name: "UosAiAssistant"
                    sourceSize: Qt.size(iconWidth , iconWidth)
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            console.log("open meeting")
                            Applet.onClickRecommend()
                        }
                    }
                }
            }
            Timer {
                id: toolTipShowTimer2
                interval: 50
                onTriggered: {
                    var point = Applet.rootObject.mapToItem(null, Applet.rootObject.width / 2, Applet.rootObject.height / 2)
                    toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
                    toolTip.open()
                }
            }

            HoverHandler {
                onHoveredChanged: {
                    if (hovered) {
                        toolTipShowTimer2.start()
                    } else {
                        if (toolTipShowTimer2.running) {
                            toolTipShowTimer2.stop()
                        }

                        toolTip.close()
                    }
                }
            }

            MouseArea {
                anchors.fill : parent
                hoverEnabled : true
                preventStealing : true
                onClicked : {
                    Applet.onClickIcon()
                }
            }

        }
    }

    Connections {
        id: dragCon
        target: Applet
        function onDragActivated(urls) {
            var needOpen = false
            if (urls.length === 1) {
                needOpen = Applet.getEnableFileDrag() && Applet.isSupportDrop(urls[0]);
            }

            if (!needOpen) {
                docToast.close()
                return
            }

            if (!docToast.popupVisible) {
                docToast.switchToTips()
                docToast.open()
            }
        }
        function onSigMeetAssistantStatusChanged(status) {
            console.log("MeetAssistantStatus==",status)
            meetAssistantStatus = status
            if (changeMeetingAssistantStatusTimer.running)
                changeMeetingAssistantStatusTimer.stop()
            changeMeetingAssistantStatusTimer.start()
        }
    }

    Timer {
        id: changeMeetingAssistantStatusTimer
        interval: 2000
        onTriggered: {
           changeMeetingAssistantStatus(meetAssistantStatus)
        }
    }

    function setItemModel() {
        var itemInfo = Applet.getItemList()
        itemModel.clear();
        for (var i in itemInfo) {
            var info = itemInfo[i]
            itemModel.append({"text" : info.text, "desc" : info.desc, "imageFile" : info.imageFile});
        }
    }

    function changeMeetingAssistantStatus(status) {
        switch (status) {
        case 1:
            iconCount = 1
            meetingIconVisible = false
            if (meetingToast.popupVisible) {
                meetingToast.close()
            }
            break;
        case 2:
            iconCount = 2
            meetingIconVisible= true
            if (meetingToast.popupVisible) {
                meetingToast.close()
            }
            break;
        case 3:
            iconCount = 2
            meetingIconVisible = true
            if (!meetingToast.popupVisible) {
                meetingToast.DockPanelPositioner.bounding = Qt.rect(iconPoint.x + meetingToast.width / 2 * ((Panel.position + 1) % 2),
                                                                    iconPoint.y + meetingToast.height / 2 * (Panel.position % 2),
                                                                    meetingToast.width, meetingToast.height)
                meetingToast.open()
            }
            break;
        default:
            break;
        }
    }

    Component.onCompleted: {
        console.info("AI Bar component initialized")
        meetAssistantStatus = Applet.getNowMeetAssistantStatus();
        changeMeetingAssistantStatus(meetAssistantStatus)
        setItemModel()
    }
    // iconlabel
}
