#ifndef NETWORK_UDP_H
#define NETWORK_UDP_H
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>
#include <QDebug>

#define MAX_ANCHOR_NUM      18
#define MAX_AGING_TIME      180

struct BaseStation {
    int groupId;
    int anchorId;
    bool status;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    QString ip;
    QString mac;

    int aging_time = MAX_AGING_TIME;     //老化时间 S

    int set_groupId = 0;
};

// 心跳数据结构
struct HeartBeatData {
    QString command;
    QString mac;
    QString firmware;
    int anchorID;
    int groupId;
    QString deviceStatus;
};

// 计算数据结构
struct AnchorData {
    QString command;
    QString mac;
    int anchorID;
    int tagID;
    int groupId;
    int battery;
    int rangeNo;
    double distance;
    int power;
    qint64 rangeTime;
    int sos;
    int alarm;
};

extern int Base_udp_num;
extern QList<BaseStation> baseStationList;

extern QList<BaseStation> loadStationList;

extern QList<AnchorData> anchorList;


#endif // NETWORK_UDP_H
