import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3


import org.deepin.dtk 1.0 as D
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

PanelPopup {
    Control {
        id:tips
        width: childrenRect.width
        height: childrenRect.height

        RowLayout {
            spacing: 7
            Layout.fillWidth : true
            Item {
                width: 1
                height: 1
            }

            Image {
                width: 16
                height: 16
                source: "qrc:/icons/deepin/builtin/icons/ai-meeting-assistant.svg"
            }

           Text {
               rightPadding: 10
               topPadding:7
               bottomPadding: 7
               //font: DTK.fontManager.t5
               text: qsTr("Enable speech-to-text, then summaries can be formed automatically after the meeting")
               verticalAlignment: Text.AlignVCenter // 垂直居中对齐文本
           }
        }

        MouseArea{
            anchors.fill: parent
            onClicked: {
                console.log("open meeting")
                Applet.onClickRecommend()
            }
        }
    }

}
