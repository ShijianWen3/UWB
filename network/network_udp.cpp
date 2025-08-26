#include "network_udp.h"
#include "RTLSDisplayApplication.h"
#include "RTLSClient.h"

#include <QMap>
#include <QList>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

// 全局数组存储数据
//QList<AnchorData> anchorList;

// 全局临时变量用于存储相同RangeNo的数据
//QList<AnchorData> anchorTempList;

// 定义一个QMap来存储基于tagID的anchorTempLists
QMap<int, QList<AnchorData>> tagIdToAnchorTempListMap;
//定义一个QMap来存储每个tagID的最后存储范围号
QMap<int, int> tagIdToLastStoredRangeNoMap;

int Base_udp_num = 0;
//全局list
QList<BaseStation> baseStationList;
//配置加载list
QList<BaseStation> loadStationList;

int lastStoredRangeNo = 0;

#define MAX_DATA_NUM				1024
extern unsigned char BufDataFromCtrl[MAX_DATA_NUM];
extern int BufCtrlPosit_w;

void RTLSClient::updateTagWarn()
{

}

QString processAnchorTempList(const QList<AnchorData> &anchorTempList, int *indexArray)
{
    QString result = "";
    int index = 0;

    //传入的anchorTempList为相同tagID
    QString tagIdHexSuffix = "a" + QString::number(anchorTempList[0].tagID) + ":0";
    QString resultLine;
    QString headline = "mc 00";
    QString tailline = "095f c1 00024c24";
    resultLine.append(headline);



    // Initialize distanceList with 8 "ffffffff" elements
    QStringList distanceList;
    distanceList << "ffffffff" << "ffffffff" << "ffffffff" << "ffffffff" << "ffffffff" << "ffffffff" << "ffffffff" << "ffffffff";

    for (const BaseStation &baseStation : baseStationList)
    {
        int anchorId = baseStation.anchorId;

        for (const AnchorData &data : anchorTempList)
        {
            if (data.groupId == baseStation.groupId && data.anchorID == anchorId)
            {
                QString distanceHex;
                int distanceInt = static_cast<int>(data.distance * 1000); // Convert to integer (e.g., 1.060 * 1000)
                distanceHex = QString("%1").arg(distanceInt, 8, 16, QChar('0'));
                int position = anchorId; // Adjust for 0-based index
                distanceList[position] = distanceHex;

                indexArray[position] = index;
                break;
            }
        }

        index++;
    }

    resultLine.append(" " + distanceList.join(" ")); // Join distanceList with " " and append to resultLine
    resultLine.append(" " + tailline);
    resultLine.append(" " + tagIdHexSuffix);
    result.append(resultLine + "\r\n");

    return result;
}

void RTLSClient::ProcessUDPData(unsigned char *BufDataFromUDP, QString &clientIP, QString &mac, int &anchorID, int &groupID)
{
    //记录基站index
    int indexArray[8];

    bool exists = false;

    memset(indexArray, 0, sizeof(indexArray));

    // 将 unsigned char 数组转换为 QString
    QString udpDataString = QString::fromUtf8(reinterpret_cast<const char*>(BufDataFromUDP));

    // 从 udpDataString 中删除 \r 和 \n 字符
    udpDataString.replace("\r", "").replace("\n", "").replace("\t", "");

    //qDebug() << "udpDataString:" << udpDataString << endl;

    // 解析 JSON 数据
    QJsonDocument doc = QJsonDocument::fromJson(udpDataString.toUtf8());
    if (!doc.isObject())
    {
        qWarning() << "Invalid JSON data.";
        return;
    }

    QJsonObject jsonObj = doc.object();

    // 检查是否为心跳数据格式
    if (jsonObj.contains("Command") && jsonObj.value("Command").isString())
    {
        QString command = jsonObj.value("Command").toString();

        // 根据不同的命令类型解析数据
        if (command == "HeartBeat")
        {
            if (jsonObj.contains("MAC") && jsonObj.value("MAC").isString() &&
                jsonObj.contains("AnchorID") && jsonObj.value("AnchorID").isDouble())
            {
                mac = jsonObj.value("MAC").toString();
                anchorID = jsonObj.value("AnchorID").toInt();
                groupID = jsonObj.value("GroupID").toInt();
//                qDebug() << "command:HeartBeat" << endl;
                for (BaseStation &baseStation : baseStationList)
                {
                    //                    if (groupID == baseStation.groupId && anchorID == baseStation.anchorId)
                    //                    {
                    //                        baseStation.status = true;
                    //                    }

                    if (baseStation.mac == mac)
                    {
                        baseStation.status = true;
                        baseStation.aging_time = MAX_AGING_TIME;
                        exists = true;
                        //如果存在，修改IP，判断gourpID与设置ID是否相同(会影响初始值，暂不修改)
                        baseStation.ip = clientIP;
                        baseStation.groupId = groupID;

                        if ((baseStation.set_groupId != groupID) && (baseStation.set_groupId != 0))
                        {
                            sendGroudIDCommandToBaseStations(clientIP, baseStation.set_groupId);
                        }

                        break;
                    }
                }

                if (!exists)
                {
                    BaseStation newBaseStation;

                    newBaseStation.mac = mac;
                    newBaseStation.anchorId = anchorID;
                    newBaseStation.ip = clientIP;
                    newBaseStation.status = true;
                    newBaseStation.groupId = groupID;
                    baseStationList.append(newBaseStation);

                    //TODO:app 3d anchor
                }

            }
        }
        else if (command == "UpLink")
        {
            AnchorData anchorData;

            if (jsonObj.contains("MAC") && jsonObj.value("MAC").isString() &&
                jsonObj.contains("AnchorID") && jsonObj.value("AnchorID").isDouble())
            {
                mac = jsonObj.value("MAC").toString();
                anchorID = jsonObj.value("AnchorID").toInt();
                groupID = jsonObj.value("GroupID").toInt();
//                qDebug() << "command:UpLink" << endl;

                for (BaseStation &baseStation : baseStationList)
                {
                    if (baseStation.mac == mac)
                    {
                        baseStation.status = true;
                        baseStation.aging_time = MAX_AGING_TIME;
                        exists = true;
                        //如果存在，修改IP，判断gourpID与设置ID是否相同(会影响初始值，暂不修改)
                        baseStation.ip = clientIP;
                        baseStation.groupId = groupID;

                        if ((baseStation.set_groupId != groupID) && (baseStation.set_groupId != 0))
                        {
                            sendGroudIDCommandToBaseStations(clientIP, baseStation.set_groupId);
                        }

                        break;
                    }
                }

                if (!exists)
                {
                    BaseStation newBaseStation;

                    newBaseStation.mac = mac;
                    newBaseStation.anchorId = anchorID;
                    newBaseStation.ip = clientIP;
                    newBaseStation.status = true;
                    newBaseStation.groupId = groupID;
                    baseStationList.append(newBaseStation);
                    //TODO:app 3d anchor
                }
            }

            anchorData.command = jsonObj["Command"].toString();
            anchorData.mac = jsonObj["MAC"].toString();
            anchorData.anchorID = jsonObj["AnchorID"].toInt();
            anchorData.tagID = jsonObj["TagID"].toInt();
            anchorData.groupId = jsonObj["GroupID"].toInt();
            anchorData.battery = jsonObj["Battery"].toInt();
            anchorData.rangeNo = jsonObj["RangeNo"].toInt();
            anchorData.distance = jsonObj["Distance"].toDouble();
            anchorData.power = jsonObj["Power"].toInt();
            anchorData.rangeTime = jsonObj["RangeTime"].toVariant().toLongLong();
            anchorData.sos = jsonObj["SOS"].toInt();
            anchorData.alarm = jsonObj["Alarm"].toInt();

            // Update tagIdToAnchorTempListMap
            if (tagIdToAnchorTempListMap.contains(anchorData.tagID))
            {
                tagIdToAnchorTempListMap[anchorData.tagID].append(anchorData);
            }
            else
            {
                QList<AnchorData> anchorList;
                anchorList.append(anchorData);
                tagIdToAnchorTempListMap.insert(anchorData.tagID, anchorList);
                //not found tagID --> rangNO
                if (!tagIdToLastStoredRangeNoMap.contains(anchorData.tagID))
                {
                    tagIdToLastStoredRangeNoMap[anchorData.tagID] = anchorData.rangeNo;
                }
            }

            int receivedRangeNo = anchorData.rangeNo % 256;

            //            qDebug() << "receivedRangeNo:" << receivedRangeNo
            //                     << "tagIdToLastStoredRangeNoMap[anchorData.tagID]:" << tagIdToLastStoredRangeNoMap[anchorData.tagID] << endl;

            if ((receivedRangeNo == 1) && (tagIdToLastStoredRangeNoMap[anchorData.tagID] == 255))
            {
                tagIdToLastStoredRangeNoMap[anchorData.tagID] = 0;
            }

            // Check and process rangeNo
            //if (receivedRangeNo == (tagIdToLastStoredRangeNoMap[anchorData.tagID] + 1))
            if (receivedRangeNo > tagIdToLastStoredRangeNoMap[anchorData.tagID] ||
                (receivedRangeNo < tagIdToLastStoredRangeNoMap[anchorData.tagID] - 8))
            {
                // Update lastStoredRangeNo for this tagID
                tagIdToLastStoredRangeNoMap[anchorData.tagID] = anchorData.rangeNo;

                //                qDebug() << "anchor List:";
                //                for (const AnchorData & Anchor : tagIdToAnchorTempListMap[anchorData.tagID])
                //                {
                //                    qDebug() << "Command: " << Anchor.command
                //                             << "MAC: " << Anchor.mac
                //                             << "AnchorID: " << Anchor.anchorID
                //                             << "TagID: " << Anchor.tagID
                //                             << "GroupID: " << Anchor.groupId
                //                             << "Battery: " << Anchor.battery
                //                             << "rangeNo: " << Anchor.rangeNo
                //                             << "Distance: " << Anchor.distance
                //                             << "Power: " << Anchor.power
                //                             << "RangeTime: " << Anchor.rangeTime
                //                             << "SOS: " << Anchor.sos
                //                             << "Alarm: " << Anchor.alarm;
                //                }

                //解析存储链表函数
                QString lastresult;
                lastresult = processAnchorTempList(tagIdToAnchorTempListMap[anchorData.tagID], indexArray);

                //                qDebug() << "lastresult:" << lastresult <<endl;

                tagIdToAnchorTempListMap[anchorData.tagID].clear();

                tagIdToAnchorTempListMap[anchorData.tagID].append(anchorData);

                // 将 QString 转换为 QByteArray
                QByteArray lastresultBytes = lastresult.toUtf8();
                // 使用 QByteArray::data() 获取指向数据的指针
                const char *dataPtr = lastresultBytes.constData();

                if (BufCtrlPosit_w + lastresultBytes.length() < MAX_DATA_NUM)
                {
                    // 使用 memcpy 复制数据到 BufDataFromCtrl
                    memcpy(BufDataFromCtrl + BufCtrlPosit_w, dataPtr, lastresultBytes.length());

                    // 更新 BufCtrlPosit_w
                    BufCtrlPosit_w += lastresultBytes.length();
                }
                else
                {
                    // 处理缓冲区溢出的情况
                    int remainingSpace = MAX_DATA_NUM - BufCtrlPosit_w;

                    memcpy(BufDataFromCtrl + BufCtrlPosit_w, dataPtr, remainingSpace);

                    int remainingData = lastresultBytes.length() - remainingSpace;
                    memcpy(BufDataFromCtrl, dataPtr + remainingSpace, remainingData);

                    BufCtrlPosit_w = remainingData;
                    if (BufCtrlPosit_w == MAX_DATA_NUM)
                    {
                        BufCtrlPosit_w = 0;
                    }
                }

                CtrlSerDataDealG(indexArray);
            }
        }
        else
        {
            qWarning() << "Invalid command type.";
        }
    }
    else
    {
        qWarning() << "Invalid JSON data format.";
    }
}

void RTLSClient::sendGroudIDCommandToBaseStations(QString ip, int GroupNo)
{
    // 创建要发送的JSON数据
    QJsonObject jsonData;
    jsonData["Command"] = "SetGroupID";
    jsonData["GroupNo"] = GroupNo;

    QUdpSocket udpSocket;

    // 将JSON数据转换为字符串
    QJsonDocument jsonDoc(jsonData);
    QByteArray data = jsonDoc.toJson(QJsonDocument::Compact);

    // 设置目标IP地址和端口
    QHostAddress targetAddress(ip);
    quint16 targetPort = 54321;

    udpSocket.writeDatagram(data, targetAddress, targetPort);
}

// void RTLSClient::sendTimeSyncCommandToBaseStations()
// {
//     // 获取当前系统时间并转换为时间戳（单位：毫秒）
//     qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

//     // 创建要发送的JSON数据
//     QJsonObject jsonData;
//     jsonData["Command"] = "TimeSync";
//     jsonData["TimeStamp"] = currentTimestamp * 1000;

//     // 遍历baseStationList并发送数据
//     for (const BaseStation &station : baseStationList)
//     {
//         QUdpSocket udpSocket;

//         if (station.status == false)
//         {
//             continue;
//         }

//         // 将JSON数据转换为字符串
//         QJsonDocument jsonDoc(jsonData);
//         QByteArray data = jsonDoc.toJson(QJsonDocument::Compact);

//         // 设置目标IP地址和端口
//         QHostAddress targetAddress(station.ip);
//         quint16 targetPort = 54321;

//         udpSocket.writeDatagram(data, targetAddress, targetPort);
//     }
// }

void RTLSClient::sendTimeSyncCommandToBaseStations()
{
    // 获取当前系统时间并转换为时间戳（单位：毫秒）
    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

    // 创建要发送的JSON数据
    QJsonObject jsonData;
    jsonData["Command"] = "TimeSync";
    jsonData["TimeStamp"] = currentTimestamp * 1000;

    // 遍历baseStationList并发送数据
//    for (const BaseStation &station : baseStationList)
//    {
//        QUdpSocket udpSocket;

//        if (station.status == false)
//        {
//            continue;
//        }

//        // 将JSON数据转换为字符串
//        QJsonDocument jsonDoc(jsonData);
//        QByteArray data = jsonDoc.toJson(QJsonDocument::Compact);

//        // 设置目标IP地址和端口
//        QHostAddress targetAddress(station.ip);
//        quint16 targetPort = 54321;

//        udpSocket.writeDatagram(data, targetAddress, targetPort);
//    }
    QUdpSocket udpSocket;
    QJsonDocument jsonDoc(jsonData);
    QByteArray data = jsonDoc.toJson(QJsonDocument::Compact);

    // 设置目标IP地址和端口
    //QHostAddress targetAddress(station.ip);
    quint16 targetPort = 54321;

    udpSocket.writeDatagram(data, QHostAddress::Broadcast, targetPort);
}


void RTLSClient::sendTagWarnCommandToBaseStations(int tagidA, bool status)
{
    // 创建要发送的JSON数据
    QJsonObject jsonDataA;
    jsonDataA["Command"] = "DownLink";
    jsonDataA["TagID"] = tagidA;
    jsonDataA["Alarm"] = status;


    for (const BaseStation &station : baseStationList)
    {
        QUdpSocket udpSocket;

        if (station.status == false)
        {
            continue;
        }

        // 设置目标IP地址和端口
        QHostAddress targetAddress(station.ip);
        quint16 targetPort = 54321;

        // 将JSON数据转换为字符串
        QJsonDocument jsonDocA(jsonDataA);
        QByteArray dataA = jsonDocA.toJson(QJsonDocument::Compact);

        udpSocket.writeDatagram(dataA, targetAddress, targetPort);
    }
}

#define UPDATA_SYS_TIME_MAX     5
int32_t update_sys_time = 0;
void RTLSClient::updateAgingTime()
{
    for (BaseStation &station : baseStationList)
    {
        if (station.aging_time > 0)
        {
            station.aging_time--; // 减少aging_time
            if (station.aging_time <= 0)
            {
                station.status = false; // 将status设置为false
            }
        }
    }

    update_sys_time++;
    if (update_sys_time == UPDATA_SYS_TIME_MAX)
    {
        update_sys_time = 0;
        sendTimeSyncCommandToBaseStations();
    }

    updateTagWarn();

    //更新表格数据
    emit updateAnchorTable(baseStationList);
}

