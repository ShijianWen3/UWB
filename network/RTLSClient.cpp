// -------------------------------------------------------------------------------------------------------------------
//
//  File: RTLSClient.cpp
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------

#include "RTLSClient.h"
#include "GraphicsWidget.h"
#include "RTLSDisplayApplication.h"
#include "SerialConnection.h"
#include "trilateration.h"
#include "network_udp.h"
#include "ViewSettingsWidget.h"

#include <QList>
#include <QTextStream>
#include <QDateTime>
#include <QThread>
#include <QFile>
#include <QDebug>
#include <math.h>
#include <QMessageBox>
#include <QUdpSocket>
#include <QDomDocument>
#include <QFile>
#include <string.h>

#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#define DEBUG_FILE 0

#define TOF_REPORT_LEN  (255)
#define TOF_REPORT_ARGS (16)

#define STATION_DATA_LEN (17)
using namespace std;

#define nDim (2)

#define MAX_DATA_NUM				1024	//传消息内容最大长度
#define DataHead    'm'      
#define DataHead2   '$'    
#define DataTail    '\n'     

//add by ec
unsigned char BufDataFromUDP[MAX_DATA_NUM] = {0};

unsigned char BufDataFromCtrl[MAX_DATA_NUM] = {0};
int BufCtrlPosit_w = 0, BufCtrlPosit_r = 0;
int DataRecord=0, rcvsign = 0;

int first_open=0;
double RTLSClient::RangeA0_A7[8];
int checkbox2d3d = 3;
QUdpSocket* mSocket;
/**
* @brief RTLSDisplayApplication
*        Constructor, it consumes the data received over the COM port connection and sends the
*        processed data to the graphical display
* */
RTLSClient::RTLSClient(QObject *parent) :
    QObject(parent),
    _first(true),
    _useAutoPos (false),
    _file(NULL),
    _fileDbg(NULL)
{

    /*
    Filter Types
    0 - No Filtering
    1 - Moving Average
    2 - Moving Average excluding max and min
    */
   // {
    _locationFilterTypes << "无" << "平均平滑滤波" << "去极值平滑滤波";
    _locationFilterTypes_en<< "null" << "Avg smoothing filter" << "De-extremum smoothing filter";
    _usingFilter = 0;

    _graphicsWidgetReady = false ;
    _tagList.clear();

    //memset(&_ancArray, 0, MAX_NUM_ANCS*sizeof(anc_struct_t));
    _serial = NULL;

    _filterSize = FILTER_SIZE_SHORT ;

    for(int a0 = 0; a0 < MAX_NUM_ANCS; a0++)
    {
        for(int b0 = 0; b0 < MAX_NUM_ANCS; b0++)
        {
            _ancRangeValues[a0][b0] = 0;
            _ancRangeValuesAvg[a0][b0] = 0;
        }
    }

    _ancRangeLastSeq = 0x0;
    _ancRangeCount = 0;

    RTLSDisplayApplication::connectReady(this, "onReady()");
    QObject::connect(RTLSDisplayApplication::serialConnection(), SIGNAL(OnUdpConnect()),this, SLOT(OnUdpConnect()));
    QObject::connect(RTLSDisplayApplication::serialConnection(), SIGNAL(OffUdpConnect()),this, SLOT(OffUdpConnect()));
    //aging time
    agingTimer = new QTimer(this);

//     _version = ver ;
//     _configuration = conf.toLocal8Bit().at(0);

    connect(agingTimer, &QTimer::timeout, this, &RTLSClient::updateAgingTime);

    agingTimer->start(1000);
    //aging time

}

void RTLSClient::onReady()
{
    QObject::connect(RTLSDisplayApplication::serialConnection(), SIGNAL(serialOpened(QString, QString)),
                         this, SLOT(onConnected(QString, QString)));
}

void RTLSClient::onConnected(QString ver, QString conf)
{
//     bool ok = true;
//     int anchor = 0, chan = 2, mode = 0, addr = 0, addr1 = 0, m = 0;
//             //tag or anchor, chan 2 or 5,

//     _first = true;
//     _version = ver ;
//     _configuration = conf.toLocal8Bit().at(0);

//    // qDebug() << "configuration " << "0x"+QString::number(_configuration, 16);
//     if(ok)
//     {
//         m = (_configuration & 6) >> 1;

//         if(m & 0x1)
//         {
//             _filterSize = FILTER_SIZE ;
//         }
//         else
//         {
//             _filterSize = FILTER_SIZE_SHORT ;
//         }

//         if(_configuration & 0x8)
//         {
//             anchor = 1;
//         }

//         mode = (_configuration & 0x2) >> 1;

//         if(_configuration & 0x4)
//         {
//             chan = 5 ;
//         }

//         addr1 = (_configuration & 0x70) >> 4;

//         addr = ((addr1 >> 2) & 0x1) + (((addr1 >> 1) & 0x1) << 1) + ((addr1 & 0x1) << 2);
//     }

//     //write the header
//     if(anchor)
//     {
//         _config = QString(":Conf:Anchor%1:%2:Chan%3\n").arg(addr).arg(mode).arg(chan);
//         emit statusBarMessage(QString("HR-RTLS Anchor ID%1 Mode %2").arg(addr).arg(m));
//         //qDebug() << QString("HR-RTLS Anchor ID%1 Mode %2").arg(addr).arg(m);

//         if(addr == 0)
//         {
//             emit enableAutoPositioning(true);
//         }
//         else
//         {
//             emit enableAutoPositioning(false);
//         }
// #if 0
//         if(addr & 0x3) //anchors 1, 2, and 3 don't support RTLS (only geo fencing)
//         {
//             QMessageBox::about(NULL, tr("COM"), QString("Connected to Anchor %1.\nRTLS is not supported, only Geo-fencing mode.").arg(addr));
//         }
//         else if (addr == 0x4)
//         {
//             QMessageBox::about(NULL, tr("COM"), QString("Connected to Listener.\nBoth RTLS and Geo-fencing modes are supported."));
//         }
//         else
//         {
//             QMessageBox::about(NULL, tr("COM"), QString("Connected to Anchor %1.\nBoth RTLS and Geo-fencing modes are supported.").arg(addr));
//         }
// #endif
//     }
//     else
//     {
//         _config = QString(":Conf:Tag%1:%2:Chan%3\n").arg(addr).arg(mode).arg(chan);
//         emit statusBarMessage(QString("HR-RTLS Tag ID%1 Mode %2").arg(addr).arg(m));
//       //  qDebug() << QString("HR-RTLS Tag ID%1 Mode %2").arg(addr).arg(m);
// #if 0
//         QMessageBox::about(NULL, tr("COM"), QString("Connected to Tag %1.").arg(addr));
// #endif
//     }
    //get pointer to Serial connection serial port pointer

    _serial = RTLSDisplayApplication::serialConnection()->serialPort();
    connect(_serial, SIGNAL(readyRead()), this, SLOT(newData()));
}
void RTLSClient::OnUdpConnect()
{
     QObject::connect(RTLSDisplayApplication::serialConnection()->udpSocket,SIGNAL(readyRead()), this, SLOT(ReceiveData()));
}
void RTLSClient::OffUdpConnect()
{
     QObject::disconnect(RTLSDisplayApplication::serialConnection()->udpSocket,SIGNAL(readyRead()), this, SLOT(ReceiveData()));
}
void RTLSClient::sendData(const QString &ip, const QJsonObject &jsonData)
{
    QUdpSocket udpSocket;

    QJsonDocument jsonDoc(jsonData);
    QByteArray data = jsonDoc.toJson(QJsonDocument::Compact);

    QHostAddress targetAddress(ip);
    quint16 targetPort = 54321;

    udpSocket.writeDatagram(data, targetAddress, targetPort);
}


void RTLSClient::ReceiveData()
{
    while(RTLSDisplayApplication::serialConnection()->udpSocket->hasPendingDatagrams())  //有未处理的报文
    {
        QByteArray recvMsg;
        QHostAddress senderAddress;
        quint16 senderPort;

        recvMsg.resize(RTLSDisplayApplication::serialConnection()->udpSocket->pendingDatagramSize());
        RTLSDisplayApplication::serialConnection()->udpSocket->readDatagram(recvMsg.data(),recvMsg.size(), &senderAddress, &senderPort);
        //qDebug()<<"recvMsg"<<recvMsg;

       int len = recvMsg.length();

        if(len > 1024)
            return;

       unsigned char *pbuf;
       unsigned char buf[2014] = {0};

        pbuf = (unsigned char *)recvMsg.data();
        memcpy(buf, pbuf, len);

        int reallength = len;
        if(reallength != 0)
        {

            memcpy(BufDataFromUDP, buf, len);

            QString clientMAC;
            int clientAnchorID = 0;
            int clientGroudID = 0;
            QString clientIP = senderAddress.toString();  // 获取客户端的 IP 地址

            //qDebug() << "clientIP:" << clientIP;

            ProcessUDPData(buf, clientIP, clientMAC, clientAnchorID, clientGroudID);
        }
        //del by ec
        //CtrlSerDataDeal();
    }
}

void RTLSClient::openLogFile(QString userfilename)
{
    qDebug(qPrintable(QString("userfilename: %1").arg(userfilename)));
    QDateTime now = QDateTime::currentDateTime();

    _logFilePath = "./Logs/"+now.toString("yyyyMMdd_hhmmss")+"RTLS1_log.txt";

    _first = true;

    _file = new QFile(_logFilePath);
#if (DEBUG_FILE==1)
    QString filenameDbg("./Logs/"+now.toString("yyyyMMdd_hhmmss")+"RTLS_log_dbg.txt");
    _fileDbg = new QFile(filenameDbg);
    if (!_fileDbg->open(QFile::ReadWrite | QFile::Text))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg(filename).arg(_file->errorString())));
        //QMessageBox::critical(NULL, tr("Logfile Error"), QString("Cannot create file %1 %2\nPlease make sure ./Logs/ folder exists.").arg(filename).arg(_file->errorString()));
    }
#endif
    if (!_file->open(QFile::ReadWrite | QFile::Text))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg(_logFilePath).arg(_file->errorString())));
        QMessageBox::critical(NULL, tr("Logfile Error"), QString("Cannot create file %1 %2\nPlease make sure ./Logs/ folder exists.").arg(_logFilePath).arg(_file->errorString()));
    }
    else
    {
        QString nowstr = now.toString("yyyy-MM-dd_hh:mm:ss:zzz ");
        //QString s = nowstr + QString("HR-RTLS1 LogFile") + _version + _config;
        QString s = nowstr + QString("HR-RTLS1 LogFile\n");
        QTextStream ts( _file );
        ts << s;
    }
}

void RTLSClient::closeLogFile(void)
{
    if(_file)
    {
        _file->flush();
        _file->close();

        _file = NULL ;
    }

#if (DEBUG_FILE==1)
    if(_fileDbg)
    {
        _fileDbg->flush();
        _fileDbg->close();

        _fileDbg = NULL ;
    }
#endif
}

const QString &RTLSClient::getLogFilePath()
{
    return _logFilePath;
}

//initialise the tag reports strusture and add to the list
void RTLSClient::initialiseTagList(int id)
{
    tag_reports_t r;
    memset(&r, 0, sizeof(tag_reports_t));
    r.id = id;
    r.ready = false;
    r.filterReady = 0;
    r.rangeSeq = -1;
    r.printStats = 0;
    memset(&r.rangeValue[0][0], -1, sizeof(r.rangeValue));
    _tagList.append(r);
}

//calculate average of last 10
double RTLSClient::process_ma(double *array, int idx)
{
    double sum = 0;
    int i, j;

    for(j=0, i=idx; j<_filterSize; i--, j++)
    {
        if(i < 0)
        {
            i = (HIS_LENGTH - 1);
        }
        sum += array[i];
    }
    return (sum / _filterSize);
}

//calculate average (of last 10) excluding min and max
double RTLSClient::process_me(double *array, int idx)
{
    double max, min, sum;
    int i, j;

    sum = max = min = array[idx];

    i=(idx--);

    if(i < 0)
    {
        i = (HIS_LENGTH - 1);
    }

    for(j=0; j<(_filterSize-1); i--, j++)
    {
        if(i < 0)
        {
            i = (HIS_LENGTH - 1);
        }
        if (array[i] > max)
        {
            max = array[i];
        }
        if (array[i] < min)
        {
            min = array[i];
        }
        sum += array[i];
    }

    sum = sum - max - min;

    return (sum / (_filterSize - 2));
}

void RTLSClient::updateTagStatistics(int i, double x, double y, double z)
//update the history array and the average
{
    QDateTime now = QDateTime::currentDateTime();
    QString nowstr = now.toString("hh:mm:ss:zzz ");
    int j = 0;
    int idx = _tagList.at(i).arr_idx;
    uint64_t id = _tagList.at(i).id;
    tag_reports_t rp = _tagList.at(i);
    double avDistanceXY = 0;
    double sum_std = 0;
    double DistanceXY[HIS_LENGTH];
    double DstCentreXY[HIS_LENGTH];
    double stdevXY = 0;

    rp.av_x = 0;
    rp.av_y = 0;
    rp.av_z = 0;


    for(j=0; j<HIS_LENGTH; j++)
    {
       rp.av_x += rp.x_arr[j];
       rp.av_y += rp.y_arr[j];
       rp.av_z += rp.z_arr[j];
    }

    rp.av_x /= HIS_LENGTH;
    rp.av_y /= HIS_LENGTH;
    rp.av_z /= HIS_LENGTH;

    for(j=0; j<HIS_LENGTH; j++)
    {
        DistanceXY[j] = sqrt((rp.x_arr[j] - rp.av_x)*(rp.x_arr[j] - rp.av_x) + (rp.y_arr[j] - rp.av_y)*(rp.y_arr[j] - rp.av_y));
    }

    for (j=0; j<HIS_LENGTH; j++)
    {
        avDistanceXY += DistanceXY[j]/HIS_LENGTH;
    }

    for(j=0; j<HIS_LENGTH; j++)
    {
        sum_std += (DistanceXY[j]-avDistanceXY)*(DistanceXY[j]-avDistanceXY);

    }

    stdevXY = sqrt(sum_std/HIS_LENGTH);

    vec2d sum_tempXY = {0, 0};
    vec2d CentrerXY = {0, 0};

    int counterXY = 0;

    for(j=0; j<HIS_LENGTH; j++)
    {
        if (DistanceXY[j] < stdevXY*2)
        {
            sum_tempXY.x += rp.x_arr[j];
            sum_tempXY.y += rp.y_arr[j];
            counterXY++;
        }

    }

    CentrerXY.x  = sum_tempXY.x/counterXY;
    CentrerXY.y  = sum_tempXY.y/counterXY;

    for(j=0; j<HIS_LENGTH; j++)
    {
        DstCentreXY[j] = sqrt((rp.x_arr[j] - CentrerXY.x)*(rp.x_arr[j] - CentrerXY.x) + (rp.y_arr[j] - CentrerXY.y)*(rp.y_arr[j] - CentrerXY.y));
    }

    r95Sort(DstCentreXY,0,HIS_LENGTH-1);

    rp.r95 = DstCentreXY[int(0.95*HIS_LENGTH)];

    //R95 = SQRT(meanErrx*meanErrx + meanErry*meanErry) + 2*SQRT(stdx*stdx+stdy*stdy)
    //rp.r95 = sqrt((rp.averr_x*rp.averr_x) + (rp.averr_y*rp.averr_y)) +
    //        2.0 * sqrt((rp.std_x*rp.std_x) + (rp.std_y*rp.std_y)) ;


    //update the value in the array
    rp.x_arr[idx] = x;
    rp.y_arr[idx] = y;
    rp.z_arr[idx] = z;

    rp.arr_idx++;
    //wrap the index
    if(rp.arr_idx >= HIS_LENGTH)
    {
        rp.arr_idx = 0;
        rp.ready = true;
        rp.filterReady = 1;

        if(first_open == 0)
        {
            _usingFilter = 2;
            first_open = 1;
        }  
    }

    rp.count++;


    if(rp.filterReady > 0)
    {
        if(_usingFilter == 2)
        {
            rp.fx = process_me(rp.x_arr, idx);
            rp.fy = process_me(rp.y_arr, idx);
            rp.fz = process_me(rp.z_arr, idx);
          //  qDebug() <<"calculate average (of last 10) excluding min and max";
        }
        else if (_usingFilter == 1)
        {
            rp.fx = process_ma(rp.x_arr, idx);
            rp.fy = process_ma(rp.y_arr, idx);
            rp.fz = process_ma(rp.z_arr, idx);
           // qDebug() <<"calculate average of last 10";
        }

        //qDebug() << rp.fx << rp.fy << rp.fz ;

        if(rp.filterReady == 1)
        {
            emit enableFiltering();
            rp.filterReady++;
        }
    }

    //update the list entry
    _tagList.replace(i, rp);

    if(rp.ready)
    {
        if(_graphicsWidgetReady)
        {
            emit tagStats(id, CentrerXY.x, CentrerXY.y, rp.av_z, rp.r95);
        }

        // if(_file)
        // {
        //     //log data to file
        //     QString s = nowstr + QString("TAG%1[%2,%3,%4]\n").arg(id).arg(rp.av_x,0,'f',3).arg(rp.av_y,0,'f',3).arg(rp.av_z,0,'f',3);
            
        //     QTextStream ts( _file );
        //     ts << s;
        // }
        rp.ready = false;
    }
}

void RTLSClient::setUseAutoPos(bool useAutoPos)
{
    _useAutoPos = useAutoPos;

    return;
}

QStringList RTLSClient::getLocationFilters(void)
{
    if(language)
    {
      return _locationFilterTypes;

    }
    else{
       return _locationFilterTypes_en;

    }
}

/*
Filter Types
0 - No Filtering
1 - Moving Average
2 - Moving Average excluding max and min
3 - Kalman Filter
*/
void RTLSClient::setLocationFilter(int filter)
{
    _usingFilter = filter ;
}


void RTLSClient::newData()
{
    QByteArray data = _serial->readAll();
    int len = data.length();

    unsigned char *pbuf;
    unsigned char buf[2014] = {0};

    if(len > 1024)
        return;

    pbuf = (unsigned char *)data.data();
    memcpy(&buf[0], pbuf, len);

    int reallength = len;
    int i;
    if(reallength != 0)
    {
        for( i=0; i < reallength; i++)
        {
            BufDataFromCtrl[BufCtrlPosit_w] = buf[i];

            BufCtrlPosit_w = (BufCtrlPosit_w==(MAX_DATA_NUM-1))? 0 : (1 + BufCtrlPosit_w);
        }
    }
    CtrlSerDataDeal();
}

void RTLSClient::SocketnewData()
{
    QByteArray data = RTLSDisplayApplication::serialConnection()->socket->readAll();
    int len = data.length();
        //    qDebug() << data;
        //    qDebug() << data.length();
    if(len > 1024)
        return;
    unsigned char *pbuf;
    unsigned char buf[2014] = {0};

    pbuf = (unsigned char *)data.data();
    memcpy(&buf[0], pbuf, len);

    int reallength = len;
    int i;
    if(reallength != 0)
    {

        for( i=0; i < reallength; i++)
        {
            BufDataFromCtrl[BufCtrlPosit_w] = buf[i];

            BufCtrlPosit_w = (BufCtrlPosit_w==(MAX_DATA_NUM-1))? 0 : (1+BufCtrlPosit_w);
        }
    }
    CtrlSerDataDeal();
}

static QList<vec3d> posList;
QList<vec3d> RTLSClient::getAnchPos()
{
    if(!posList.isEmpty())
    {
        posList.clear();
    }
    vec3d pos;
    for (int i = 0; i < MAX_ANC_NUM; i++) 
    {
        //if(stationList[i])
        {
            pos.x = _ancArray[i].x;
            pos.y = _ancArray[i].y;
            pos.z = _ancArray[i].z;
            posList.append(pos);
        }
    }
    return posList;
}
bool reseveSetok = false;
void RTLSClient::CtrlSerDataDealG(int *indexArray)
{
    unsigned char middata = 0;
    static unsigned char dataTmp[MAX_DATA_NUM] = {0};

    while(BufCtrlPosit_r != BufCtrlPosit_w)
    {
        middata = BufDataFromCtrl[BufCtrlPosit_r];
        BufCtrlPosit_r = (BufCtrlPosit_r==MAX_DATA_NUM-1)? 0 : (BufCtrlPosit_r+1);

        if(((middata == DataHead)||(middata == DataHead2))&&(rcvsign == 0))//收到头
        {
            rcvsign = 1;//开始了一个数据帧
            dataTmp[DataRecord++] = middata;//数据帧接收中
        }
        else if((middata != DataTail)&&(rcvsign == 1))
        {
            dataTmp[DataRecord++] = middata;//数据帧接收中
            //qDebug("middata != DataTail dataTmp = %s\n", dataTmp);
        }
        else if((middata == DataTail)&&(rcvsign == 1))//收到尾
        {
            if(DataRecord != 1)
            {
                rcvsign = 0;
                dataTmp[DataRecord++] = middata;
                dataTmp[DataRecord] = '\0';

//                qDebug("data = %s", dataTmp);
//                qDebug() << "数据长度：" << DataRecord;

                const char str[] = "$setok";
                if(strstr((const char*)dataTmp,str))//返回指向第一次出现str2位置的指针，如果没找到则返回NULL
                {
                    reseveSetok = true;
                }
                /*调用处理函数*/
                if(dataTmp[0] == 'm')
                {
                    CheckCommFrameG((const char*)(char*)dataTmp, DataRecord, indexArray);
                }

                //    static long i = 0;
                //qDebug("data = %s", dataTmp);
                //qDebug() << "计数："<< i << "数据长度：" << DataRecord;
                //    i++;
                DataRecord = 0;
            }
        }
    }
}


void RTLSClient::CtrlSerDataDeal()
{
    unsigned char middata = 0;
    static unsigned char dataTmp[MAX_DATA_NUM] = {0};

    while(BufCtrlPosit_r != BufCtrlPosit_w)
    {
        middata = BufDataFromCtrl[BufCtrlPosit_r];
        BufCtrlPosit_r = (BufCtrlPosit_r==MAX_DATA_NUM-1)? 0 : (BufCtrlPosit_r+1);

        if(((middata == DataHead)||(middata == DataHead2))&&(rcvsign == 0))//收到头
        {
            rcvsign = 1;//开始了一个数据帧
            dataTmp[DataRecord++] = middata;//数据帧接收中
        }
        else if((middata != DataTail)&&(rcvsign == 1))
        {
            dataTmp[DataRecord++] = middata;//数据帧接收中
        }
        else if((middata == DataTail)&&(rcvsign == 1))//收到尾
        {
            if(DataRecord != 1)
            {
                rcvsign = 0;
                dataTmp[DataRecord++] = middata;
                dataTmp[DataRecord] = '\0';

//                qDebug("data = %s", dataTmp);
//                qDebug() << "数据长度：" << DataRecord;

                const char str[] = "$setok";
                if(strstr((const char*)dataTmp,str))//返回指向第一次出现str2位置的指针，如果没找到则返回NULL
                {
                    reseveSetok = true;
                }
                /*调用处理函数*/
                if(dataTmp[0] == 'm')
                {
                    CheckCommFrame((const char*)(char*)dataTmp, DataRecord);
                }

                //    static long i = 0;
                //    qDebug("data = %s", dataTmp);
                //    qDebug() << "计数："<< i << "数据长度：" << DataRecord;
                //    i++;
                DataRecord = 0;
            }
        }
    }
}

int matrixBuff[MAX_NUM_ANCS][MAX_NUM_ANCS] = {0};
int step=0;
int aid_last = 0;
int errNum = 0; //记录连续标记失败次数
bool signalTimeOut = false;
#define MAX_ERR_NUM (3)

void RTLSClient::CheckCommFrameG(const char* pbuf, int revLen, int *indexArray )//串口有效数据校验
{
    QDateTime now = QDateTime::currentDateTime();
    QString nowstr = now.toString("hh:mm:ss:zzz ");
    QString statusMsg;

    int aid=0, tid=0, lnum=0, seq=0, mask=0;
    int range[8] = {-1};
    int rangetime;
    char c, type;
    int rx_power = 800;
    int mask_table[8] = {0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000, 0b00100000, 0b01000000, 0b10000000};
    int alarm;

    //mc ff ffffffff ffffffff 00002134 000005dc ffffffff ffffffff ffffffff ffffffff 095f c1 00024c24 a01:0 1a9a
    //mc ff 00001509 000024da 000024da 00001509 095f c1 00024c24 a01:0 1a9a
    if(pbuf[1] == 'c')
    {
        if((revLen == 107) || (revLen == 106))//8基站，带rxpower(107字节)
        {
            int n = sscanf(pbuf,"m%c %x %x %x %x %x %x %x %x %x %x %x %x %c%d:%d %x", &type, &alarm, &range[0], &range[1], &range[2], &range[3], &range[4], &range[5], &range[6], &range[7], &lnum, &seq, &rangetime, &c, &tid, &aid, &rx_power);
        }
        else if((revLen == 71) || (revLen == 70)) //4基站，带rxpower(71字节)
        {
            int n = sscanf(pbuf,"m%c %x %x %x %x %x %x %x %x %c%d:%d %x", &type, &alarm, &range[0], &range[1], &range[2], &range[3], &lnum, &seq, &rangetime, &c, &tid, &aid, &rx_power);
        }
        else if((revLen == 66)||(revLen == 65)) //4基站，不带rxpower(66字节)
        {
            int n = sscanf(pbuf,"m%c %x %x %x %x %x %x %x %x %c%d:%d", &type, &alarm, &range[0], &range[1], &range[2], &range[3], &lnum, &seq, &rangetime, &c, &tid, &aid);
        }
        else if((revLen == 102)||(revLen == 101))//8基站，不带rxpower(102字节)
        {
            int n = sscanf(pbuf,"m%c %x %x %x %x %x %x %x %x %x %x %x %x %c%d:%d", &type, &alarm, &range[0], &range[1], &range[2], &range[3], &range[4], &range[5], &range[6], &range[7], &lnum, &seq, &rangetime, &c, &tid, &aid);
        }
        else
        {
            return;
            //qDebug() << "err lengh="<< revLen;
        }
//        qInfo()<<" type: "<<type<<", alarm: "<<alarm<<", range[0]: "<<range[0]<<", range[1]: "<<range[1]<<", range[2]: "<<range[2]<<", range[3]: "<<range[3]<<", range[4]: "<<range[4] \
//                <<", range[5]: "<<range[5]<<", range[6]: "<<range[6]<<", range[7]: "<<range[7]<<", lnum: "<<lnum<<", seq: "<<seq<<", rangetime: "<<rangetime<<", c: "<<c \
//                <<", tid: "<<tid<<", aid: "<<aid<<", rx_power: "<<rx_power<<QTime::currentTime();
        for(int i = 0; i < 8; i++)
        {
            if(range[i] > 0)
            {
                mask = mask | mask_table[i];
            }
        }

//        qDebug("mask = 0x%x\n", mask);

        //notify the user if connected to a tag or an anchor
        if(_first)
        {
            if(c == 'a') //we are connected to an anchor id = i
            {
                statusMsg = "Connected to Anchor ID" + QString(" %1.").arg(aid);

                emit statusBarMessage(statusMsg);
            }
            else
            if(c == 't') //we are connected to a tag id = i
            {
                statusMsg = "Connected to Tag ID" + QString(" %1.").arg(tid);

                emit statusBarMessage(statusMsg);
            }
            else
            if(c == 'l') //we are connected to a listener id = i
            {
                statusMsg = "Connected to Listener ID" + QString(" %1.").arg(aid);

                emit statusBarMessage(statusMsg);
            }
            //log the anchor co-ordinates to the file
            for(int j=0; j<MAX_NUM_ANCS; j++)
            {
                if(_file)
                {
                    QString s =  nowstr + QString("Anchor%1(%2,%3,%4)\n").arg(j).arg(_ancArray[j].x).arg(_ancArray[j].y).arg(_ancArray[j].z);
                    QTextStream ts( _file );
                    ts << s;
                }
            }
            _first = false;
        }
    }
    //mi,981.937,0.63,NULL,NULL,NULL,-2.777783,1.655664,9.075048,-0.004788,-0.014364,-0.001596,T0     //13
    //mi,3.710,0.55,NULL,NULL,NULL,NULL,NULL,NULL,NULL,-1.327881,0.653174,9.577490,-0.004788,-0.013300,-0.002128,T0    //17
    else if(pbuf[1] == 'i')
    {
        float rangetime2;
        float acc[3], gyro[3];

        char *ptr, *retptr;
        ptr = (char*)pbuf;
        char cut_data[30][12];
        int cut_count = 0;

        while((retptr = strtok(ptr,",")) != NULL )
        {
            //printf("%s\n", retptr);
            strcpy(cut_data[cut_count], retptr);
            ptr = NULL;
            cut_count++;
            if(cut_count >= 29)
                break;
        }

        rangetime2 = atof(cut_data[1]);
        rangetime = rangetime2 * 1000;
        
        if(cut_count == 13)  //4anchors
        {
            for(int i = 0; i < 4; i++)
            {
                //if(strcmp(cut_data[i+2], "NULL") || strcmp(cut_data[i+2], "null"))
                if((strcmp(cut_data[i+2], "null") != 0) && (strcmp(cut_data[i+2], "NULL") != 0))
                {
                    range[i] = atof(cut_data[i+2]) * 1000;
                    mask = mask | mask_table[i];
                }
                else
                {
                    range[i] = -1;
                }
            }

            for(int i = 0; i < 3; i++)
            {
                acc[i] = atof(cut_data[i+6]);
            }

            for(int i = 0; i < 3; i++)
            {
                gyro[i] = atof(cut_data[i+9]);
            }

            tid = atoi((char*)cut_data[12]+1);

            // qDebug("mask = 0x%02X\n", mask);
            // qDebug("rangetime2 = %.3f\n", rangetime2);
            // qDebug("range[0] = %d\n", range[0]);
            // qDebug("range[1] = %d\n", range[1]);
            // qDebug("range[2] = %d\n", range[2]);
            // qDebug("range[3] = %d\n", range[3]);
            // qDebug("acc[0] = %.3f\n", acc[0]);
            // qDebug("acc[1] = %.3f\n", acc[1]);
            // qDebug("acc[2] = %.3f\n", acc[2]);
            // qDebug("gyro[0] = %.3f\n", gyro[0]);
            // qDebug("gyro[1] = %.3f\n", gyro[1]);
            // qDebug("gyro[2] = %.3f\n", gyro[2]);
            // qDebug("tid = %d\n", tid);
        }
        else if(cut_count == 17)  //8anchors
        {
            for(int i = 0; i < 8; i++)
            {
                if((strcmp(cut_data[i+2], "null") != 0) && (strcmp(cut_data[i+2], "NULL") != 0))
                {
                    range[i] = atof(cut_data[i+2]) * 1000;
                    mask = mask | mask_table[i];
                }
                else
                {
                    range[i] = -1;
                }
            }

            for(int i = 0; i < 3; i++)
            {
                acc[i] = atof(cut_data[i+6+4]);
            }

            for(int i = 0; i < 3; i++)
            {
                gyro[i] = atof(cut_data[i+9+4]);
            }
            tid = atoi((char*)cut_data[16]+1);

            // qDebug("mask = 0x%02X\n", mask);
            // qDebug("rangetime2 = %.3f\n", rangetime2);
            // qDebug("range[0] = %d\n", range[0]);
            // qDebug("range[1] = %d\n", range[1]);
            // qDebug("range[2] = %d\n", range[2]);
            // qDebug("range[3] = %d\n", range[3]);
            // qDebug("range[4] = %d\n", range[4]);
            // qDebug("range[5] = %d\n", range[5]);
            // qDebug("range[6] = %d\n", range[6]);
            // qDebug("range[7] = %d\n", range[7]);
            // qDebug("acc[0] = %.3f\n", acc[0]);
            // qDebug("acc[1] = %.3f\n", acc[1]);
            // qDebug("acc[2] = %.3f\n", acc[2]);
            // qDebug("gyro[0] = %.3f\n", gyro[0]);
            // qDebug("gyro[1] = %.3f\n", gyro[1]);
            // qDebug("gyro[2] = %.3f\n", gyro[2]);
            // qDebug("tid = %d\n", tid);
        }
        else
        {
            return;
        }
    }
    else if(pbuf[1] == 'a')
    {
        if((revLen == 106))
        {
            _autoPosRunning = true;
            int n = sscanf(pbuf,"m%c %x %x %x %x %x %x %x %x %x %x %x %x %c%d:%d %x",
                           &type, &alarm,
                           &range[0], &range[1],
                           &range[2], &range[3],
                           &range[4], &range[5],
                           &range[6], &range[7],
                           &lnum, &seq, &rangetime, &c, &tid, &aid, &rx_power);
            if(n != STATION_DATA_LEN || n == EOF)
            {
                qInfo()<<" data Error !!!!!!!!!!!!!!!! ";
                return;
            }
        }
        else
        {
            qDebug("len=%d err!", revLen);
            return;
        }
//        qInfo()<<" type: "<<type<<", alarm: "<<alarm<<", range[0]: "<<range[0]<<", range[1]: "<<range[1]<<", range[2]: "<<range[2]<<", range[3]: "<<range[3]<<", range[4]: "<<range[4] \
//            <<", range[5]: "<<range[5]<<", range[6]: "<<range[6]<<", range[7]: "<<range[7]<<", lnum: "<<lnum<<", seq: "<<seq<<", rangetime: "<<rangetime<<", c: "<<c \
//                <<", tid: "<<tid<<", aid: "<<aid<<", rx_power: "<<", "<<rx_power<<", stationNumber: "<<stationNumber<<", maxStationID: "<<maxStationID<<", aid_last: "<<aid_last<<", aid: "<<aid;

        //将接收到的基站数据进行备份
        matrixBuff[0][aid] = range[0];
        for (int i = 0; i < MAX_NUM_ANCS; i++)
        {
            matrixBuff[aid][i] = range[i];
        }

        qInfo()<<" aid: "<<aid<<", aid_last: "<<aid_last;
        if(aid != aid_last)
        {
            if(aid <= aid_last)
            {
                //一组数据接收完毕，开始进行基站坐标解算
                QVector<int> reveiveAID;
                static bool isFirst = true;

                //校验buff数据完整性，是否符合已勾选基站数量
                for (int i = 0; i < MAX_NUM_ANCS; i++)
                {
                    //该基站是否勾选
                    //if(stationList[i])
                    {
                        for (int j = 0; j < MAX_NUM_ANCS; j++)
                        {
                            if(matrixBuff[i][j] != 0)
                            {
                                //数据有效，无需继续遍历。将当前aid存入reveiveAID
                                reveiveAID.push_back(i);
                                break;
                            }
                        }
                    }
                }

                if(reveiveAID.size() == stationNumber)   //有效信号数量与已勾选基站数一致
                {
                    //将buffer数据填充到矩阵中
                    for (int i = 0; i < stationNumber; i++)
                    {
                        for (int j = 0; j < stationNumber; j++)
                        {
                            g_range_anchor[i][j] = matrixBuff[reveiveAID[i]][reveiveAID[j]];

                            //对多次矩阵数据做平均值处理
                            if(isFirst)
                            {
                                //首次处理信号，不做平均值处理
                                g_range_anchor_avg[i][j] = g_range_anchor[i][j];
                            }
                            else
                            {
                                //方案1：使用本次信号与上次信号的平均值
                                g_range_anchor_avg[i][j] = (double)((g_range_anchor[i][j]+g_range_anchor_backup[i][j])/2.0f);
                                //方案2：使用本次信号与上次平均值的平均值
                                //                                g_range_anchor_avg[i][j] = (double)((g_range_anchor[i][j]+g_range_anchor_avg[i][j])/2.0f);
                            }

                            g_range_anchor_backup[i][j] = g_range_anchor[i][j];
                        }
                    }

                    //we have 3 ranges (A0 to A1, A0 to A2 and A1 to A2)
                    //calculate A0, A1 and A2 coordinates (x, y) based on A0 = 0,0 and assume A2 is to the right of A1
                    //            qDebug("finish\n");
                    int nNodes = stationNumber;
                    int viewNode = 0;
                    mat twrdistance = zeros<mat>(nNodes,nNodes);
                    mat transCoord = zeros<mat>(nNodes,nDim);
                    mat estCoord = zeros<mat>(nNodes,nDim);


                    for(int i=0;i<stationNumber;i++)
                    {
                        for(int j=0;j<stationNumber;j++){
                            if(i==0)
                                _ancRangeValuesAvg[i][j]=g_range_anchor_avg[j][i]/1000;
                            else
                                _ancRangeValuesAvg[i][j]=g_range_anchor_avg[i][j]/1000;
                        }
                    }

                    for (int i = 0; i<nNodes; i++)   //change the array to mat format
                    {
                        for (int j = 0; j<nNodes; j++)
                        {

                            twrdistance(i,j) =  _ancRangeValuesAvg[i][j];
                            //                    qDebug("a%d - a%d = %d",i,j,(int)(_ancRangeValuesAvg[i][j]*1000));
                        }
                    }

                    mds(twrdistance, nNodes, viewNode, &transCoord); // Using multi-dimension scaling to estimation the shape

                    angleRotation(transCoord, nNodes, &estCoord); // estCoord is the coordinates with a1 on the x axis and a2 in the first quadrant

                    //emit ancRanges(_ancRangeValues[0][1], _ancRangeValues[0][2], _ancRangeValues[1][2]); //send the update to graphic
                    //for(int i=0; i<(MAX_NUM_ANCS-1); i++) // Only update positions of A0, A1 and A2
                    for(int i=0; i < reveiveAID.size(); i++) //  update positions of A0, A1 and A2 A3
                    {
                        //_ancArray[reveiveAID[i]].x = estCoord.at(i,0);
                        //_ancArray[reveiveAID[i]].y = estCoord.at(i,1);

                        //                qDebug("a%d = [%f,%f]", i, _ancArray[i].x, _ancArray[i].y);

                        //emit anchPos(reveiveAID[i], _ancArray[reveiveAID[i]].x, _ancArray[reveiveAID[i]].y, _ancArray[reveiveAID[i]].z, false, true); // Update table entry
                    }
                    isFirst = false;
                    myGraphicsWidget->calibrationSuccess();
                    errNum = 0;
                }
                else
                {
                    //有效数据缺失，需提醒标定失败
                    errNum++;
                    myGraphicsWidget->calibrationSuccess();//错误数<3: 抛弃本次数据，开启下一次自标定，无弹出提示
                    if(errNum >= MAX_ERR_NUM)
                    {
                        QString err_aid = "";
                        for (int i = 0; i < MAX_NUM_ANCS; i++)
                        {
                            //if(stationList[i] == true)
                            {
                                for (int j = 0; j < MAX_NUM_ANCS; j++)
                                {
                                    if(matrixBuff[i][j] != 0)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        if(j == MAX_NUM_ANCS-1)
                                        {
                                            err_aid += ("A"+QString::number(i)+" ");
                                        }
                                    }
                                }
                            }
                        }
                        myGraphicsWidget->calibrationFailed();
                        if(language)
                        {
                            QMessageBox::warning(NULL,"警告","自标定失败\n请检查"+err_aid+"基站是否在线","我知道了");
                        }
                        else{
                            QMessageBox::warning(NULL,"Warning","Self-calibration failure Please check whether the "+err_aid+" station is online.","OK");
                        }

                        errNum = 0;
                    }
                }
                for (int i = 0; i < MAX_NUM_ANCS; i++)
                {
                    for (int j = 0; j < MAX_NUM_ANCS; j++)
                    {
                        matrixBuff[i][j] = 0;
                    }
                }
            }
            aid_last = aid;
        }
        return;
    }
    else
    {
        return;
    }

    int idx = processTagRangeReports(tid, range, lnum, seq, mask, rx_power); //this is received when tags range to anchors
    if(idx != -1)
        trilaterateTagG(tid, seq, idx, indexArray);
}

void RTLSClient::CheckCommFrame(const char* pbuf, int revLen)//串口有效数据校验
{
    QDateTime now = QDateTime::currentDateTime();
    QString nowstr = now.toString("hh:mm:ss:zzz ");
    QString statusMsg;

    int aid=0, tid=0, lnum=0, seq=0, mask=0;
    int range[8] = {-1};
    int rangetime;
    char c, type;
    int rx_power = 800;
    int mask_table[8] = {0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000, 0b00100000, 0b01000000, 0b10000000};
    int alarm;

    //mc ff ffffffff ffffffff 00002134 000005dc ffffffff ffffffff ffffffff ffffffff 095f c1 00024c24 a01:0 1a9a
    //mc ff 00001509 000024da 000024da 00001509 095f c1 00024c24 a01:0 1a9a
    if(pbuf[1] == 'c')
    {
        if((revLen == 107) || (revLen == 106))//8基站，带rxpower(107字节)
        {
            int n = sscanf(pbuf,"m%c %x %x %x %x %x %x %x %x %x %x %x %x %c%d:%d %x", &type, &alarm, &range[0], &range[1], &range[2], &range[3], &range[4], &range[5], &range[6], &range[7], &lnum, &seq, &rangetime, &c, &tid, &aid, &rx_power);
        }
        else if((revLen == 71) || (revLen == 70)) //4基站，带rxpower(71字节)
        {
            int n = sscanf(pbuf,"m%c %x %x %x %x %x %x %x %x %c%d:%d %x", &type, &alarm, &range[0], &range[1], &range[2], &range[3], &lnum, &seq, &rangetime, &c, &tid, &aid, &rx_power);
        }
        else if((revLen == 66)||(revLen == 65)) //4基站，不带rxpower(66字节)
        {
            int n = sscanf(pbuf,"m%c %x %x %x %x %x %x %x %x %c%d:%d", &type, &alarm, &range[0], &range[1], &range[2], &range[3], &lnum, &seq, &rangetime, &c, &tid, &aid);
        }
        else if((revLen == 102)||(revLen == 101))//8基站，不带rxpower(102字节)
        {
            int n = sscanf(pbuf,"m%c %x %x %x %x %x %x %x %x %x %x %x %x %c%d:%d", &type, &alarm, &range[0], &range[1], &range[2], &range[3], &range[4], &range[5], &range[6], &range[7], &lnum, &seq, &rangetime, &c, &tid, &aid);
        }
        else
        {
            return;
            //qDebug() << "err lengh="<< revLen;
        }
//        qInfo()<<" type: "<<type<<", alarm: "<<alarm<<", range[0]: "<<range[0]<<", range[1]: "<<range[1]<<", range[2]: "<<range[2]<<", range[3]: "<<range[3]<<", range[4]: "<<range[4] \
//                <<", range[5]: "<<range[5]<<", range[6]: "<<range[6]<<", range[7]: "<<range[7]<<", lnum: "<<lnum<<", seq: "<<seq<<", rangetime: "<<rangetime<<", c: "<<c \
//                <<", tid: "<<tid<<", aid: "<<aid<<", rx_power: "<<rx_power<<QTime::currentTime();
        for(int i = 0; i < 8; i++)
        {
            if(range[i] > 0)
            {
                mask = mask | mask_table[i];
            }
        }

//        qDebug("mask = 0x%x\n", mask);

        //notify the user if connected to a tag or an anchor
        if(_first)
        {
            if(c == 'a') //we are connected to an anchor id = i
            {
                statusMsg = "Connected to Anchor ID" + QString(" %1.").arg(aid);

                emit statusBarMessage(statusMsg);
            }
            else
            if(c == 't') //we are connected to a tag id = i
            {
                statusMsg = "Connected to Tag ID" + QString(" %1.").arg(tid);

                emit statusBarMessage(statusMsg);
            }
            else
            if(c == 'l') //we are connected to a listener id = i
            {
                statusMsg = "Connected to Listener ID" + QString(" %1.").arg(aid);

                emit statusBarMessage(statusMsg);
            }
            //log the anchor co-ordinates to the file
            for(int j=0; j < MAX_NUM_ANCS; j++)
            {
                if(_file)
                {
                    QString s =  nowstr + QString("Anchor%1(%2,%3,%4)\n").arg(j).arg(_ancArray[j].x).arg(_ancArray[j].y).arg(_ancArray[j].z);
                    QTextStream ts( _file );
                    ts << s;
                }
            }
            _first = false;
        }
    }
    //mi,981.937,0.63,NULL,NULL,NULL,-2.777783,1.655664,9.075048,-0.004788,-0.014364,-0.001596,T0     //13
    //mi,3.710,0.55,NULL,NULL,NULL,NULL,NULL,NULL,NULL,-1.327881,0.653174,9.577490,-0.004788,-0.013300,-0.002128,T0    //17
    else if(pbuf[1] == 'i')
    {
        float rangetime2;
        float acc[3], gyro[3];

        char *ptr, *retptr;
        ptr = (char*)pbuf;
        char cut_data[30][12];
        int cut_count = 0;

        while((retptr = strtok(ptr,",")) != NULL )
        {
            //printf("%s\n", retptr);
            strcpy(cut_data[cut_count], retptr);
            ptr = NULL;
            cut_count++;
            if(cut_count >= 29)
                break;
        }

        rangetime2 = atof(cut_data[1]);
        rangetime = rangetime2 * 1000;
        
        if(cut_count == 13)  //4anchors
        {
            for(int i = 0; i < 4; i++)
            {
                //if(strcmp(cut_data[i+2], "NULL") || strcmp(cut_data[i+2], "null"))
                if((strcmp(cut_data[i+2], "null") != 0) && (strcmp(cut_data[i+2], "NULL") != 0))
                {
                    range[i] = atof(cut_data[i+2]) * 1000;
                    mask = mask | mask_table[i];
                }
                else
                {
                    range[i] = -1;
                }
            }

            for(int i = 0; i < 3; i++)
            {
                acc[i] = atof(cut_data[i+6]);
            }

            for(int i = 0; i < 3; i++)
            {
                gyro[i] = atof(cut_data[i+9]);
            }

            tid = atoi((char*)cut_data[12]+1);

            // qDebug("mask = 0x%02X\n", mask);
            // qDebug("rangetime2 = %.3f\n", rangetime2);
            // qDebug("range[0] = %d\n", range[0]);
            // qDebug("range[1] = %d\n", range[1]);
            // qDebug("range[2] = %d\n", range[2]);
            // qDebug("range[3] = %d\n", range[3]);
            // qDebug("acc[0] = %.3f\n", acc[0]);
            // qDebug("acc[1] = %.3f\n", acc[1]);
            // qDebug("acc[2] = %.3f\n", acc[2]);
            // qDebug("gyro[0] = %.3f\n", gyro[0]);
            // qDebug("gyro[1] = %.3f\n", gyro[1]);
            // qDebug("gyro[2] = %.3f\n", gyro[2]);
            // qDebug("tid = %d\n", tid);
        }
        else if(cut_count == 17)  //8anchors
        {
            for(int i = 0; i < 8; i++)
            {
                if((strcmp(cut_data[i+2], "null") != 0) && (strcmp(cut_data[i+2], "NULL") != 0))
                {
                    range[i] = atof(cut_data[i+2]) * 1000;
                    mask = mask | mask_table[i];
                }
                else
                {
                    range[i] = -1;
                }
            }

            for(int i = 0; i < 3; i++)
            {
                acc[i] = atof(cut_data[i+6+4]);
            }

            for(int i = 0; i < 3; i++)
            {
                gyro[i] = atof(cut_data[i+9+4]);
            }
            tid = atoi((char*)cut_data[16]+1);

            // qDebug("mask = 0x%02X\n", mask);
            // qDebug("rangetime2 = %.3f\n", rangetime2);
            // qDebug("range[0] = %d\n", range[0]);
            // qDebug("range[1] = %d\n", range[1]);
            // qDebug("range[2] = %d\n", range[2]);
            // qDebug("range[3] = %d\n", range[3]);
            // qDebug("range[4] = %d\n", range[4]);
            // qDebug("range[5] = %d\n", range[5]);
            // qDebug("range[6] = %d\n", range[6]);
            // qDebug("range[7] = %d\n", range[7]);
            // qDebug("acc[0] = %.3f\n", acc[0]);
            // qDebug("acc[1] = %.3f\n", acc[1]);
            // qDebug("acc[2] = %.3f\n", acc[2]);
            // qDebug("gyro[0] = %.3f\n", gyro[0]);
            // qDebug("gyro[1] = %.3f\n", gyro[1]);
            // qDebug("gyro[2] = %.3f\n", gyro[2]);
            // qDebug("tid = %d\n", tid);
        }
        else
        {
            return;
        }
    }
    else if(pbuf[1] == 'a')
    {
        if((revLen == 106))
        {
            _autoPosRunning = true;
            int n = sscanf(pbuf,"m%c %x %x %x %x %x %x %x %x %x %x %x %x %c%d:%d %x",
                           &type, &alarm,
                           &range[0], &range[1],
                           &range[2], &range[3],
                           &range[4], &range[5],
                           &range[6], &range[7],
                           &lnum, &seq, &rangetime, &c, &tid, &aid, &rx_power);
            if(n != STATION_DATA_LEN || n == EOF)
            {
                qInfo()<<" data Error !!!!!!!!!!!!!!!! ";
                return;
            }
        }
        else
        {
            qDebug("len=%d err!", revLen);
            return;
        }
//        qInfo()<<" type: "<<type<<", alarm: "<<alarm<<", range[0]: "<<range[0]<<", range[1]: "<<range[1]<<", range[2]: "<<range[2]<<", range[3]: "<<range[3]<<", range[4]: "<<range[4] \
//            <<", range[5]: "<<range[5]<<", range[6]: "<<range[6]<<", range[7]: "<<range[7]<<", lnum: "<<lnum<<", seq: "<<seq<<", rangetime: "<<rangetime<<", c: "<<c \
//                <<", tid: "<<tid<<", aid: "<<aid<<", rx_power: "<<", "<<rx_power<<", stationNumber: "<<stationNumber<<", maxStationID: "<<maxStationID<<", aid_last: "<<aid_last<<", aid: "<<aid;

        //将接收到的基站数据进行备份
        matrixBuff[0][aid] = range[0];
        for (int i = 0; i < MAX_NUM_ANCS; i++)
        {
            matrixBuff[aid][i] = range[i];
        }

        qInfo()<<" aid: "<<aid<<", aid_last: "<<aid_last;
        if(aid != aid_last)
        {
            if(aid <= aid_last)
            {
                //一组数据接收完毕，开始进行基站坐标解算
                QVector<int> reveiveAID;
                static bool isFirst = true;

                //校验buff数据完整性，是否符合已勾选基站数量
                for (int i = 0; i < MAX_NUM_ANCS; i++)
                {
                    //该基站是否勾选
                    //if(stationList[i])
                    {
                        for (int j = 0; j < MAX_NUM_ANCS; j++)
                        {
                            if(matrixBuff[i][j] != 0)
                            {
                                //数据有效，无需继续遍历。将当前aid存入reveiveAID
                                reveiveAID.push_back(i);
                                break;
                            }
                        }
                    }
                }

                if(reveiveAID.size() == stationNumber)   //有效信号数量与已勾选基站数一致
                {
                    //将buffer数据填充到矩阵中
                    for (int i = 0; i < stationNumber; i++)
                    {
                        for (int j = 0; j < stationNumber; j++)
                        {
                            g_range_anchor[i][j] = matrixBuff[reveiveAID[i]][reveiveAID[j]];

                            //对多次矩阵数据做平均值处理
                            if(isFirst)
                            {
                                //首次处理信号，不做平均值处理
                                g_range_anchor_avg[i][j] = g_range_anchor[i][j];
                            }
                            else
                            {
                                //方案1：使用本次信号与上次信号的平均值
                                g_range_anchor_avg[i][j] = (double)((g_range_anchor[i][j]+g_range_anchor_backup[i][j])/2.0f);
                                //方案2：使用本次信号与上次平均值的平均值
                                //                                g_range_anchor_avg[i][j] = (double)((g_range_anchor[i][j]+g_range_anchor_avg[i][j])/2.0f);
                            }

                            g_range_anchor_backup[i][j] = g_range_anchor[i][j];
                        }
                    }

                    //we have 3 ranges (A0 to A1, A0 to A2 and A1 to A2)
                    //calculate A0, A1 and A2 coordinates (x, y) based on A0 = 0,0 and assume A2 is to the right of A1
                    //            qDebug("finish\n");
                    int nNodes = stationNumber;
                    int viewNode = 0;
                    mat twrdistance = zeros<mat>(nNodes,nNodes);
                    mat transCoord = zeros<mat>(nNodes,nDim);
                    mat estCoord = zeros<mat>(nNodes,nDim);


                    for(int i=0;i<stationNumber;i++)
                    {
                        for(int j=0;j<stationNumber;j++){
                            if(i==0)
                                _ancRangeValuesAvg[i][j]=g_range_anchor_avg[j][i]/1000;
                            else
                                _ancRangeValuesAvg[i][j]=g_range_anchor_avg[i][j]/1000;
                        }
                    }

                    for (int i = 0; i<nNodes; i++)   //change the array to mat format
                    {
                        for (int j = 0; j<nNodes; j++)
                        {

                            twrdistance(i,j) =  _ancRangeValuesAvg[i][j];
                            //                    qDebug("a%d - a%d = %d",i,j,(int)(_ancRangeValuesAvg[i][j]*1000));
                        }
                    }

                    mds(twrdistance, nNodes, viewNode, &transCoord); // Using multi-dimension scaling to estimation the shape

                    angleRotation(transCoord, nNodes, &estCoord); // estCoord is the coordinates with a1 on the x axis and a2 in the first quadrant

                    //emit ancRanges(_ancRangeValues[0][1], _ancRangeValues[0][2], _ancRangeValues[1][2]); //send the update to graphic
                    //for(int i=0; i<(MAX_NUM_ANCS-1); i++) // Only update positions of A0, A1 and A2
                    for(int i=0; i < reveiveAID.size(); i++) //  update positions of A0, A1 and A2 A3
                    {
                       // _ancArray[reveiveAID[i]].x = estCoord.at(i,0);
                       // _ancArray[reveiveAID[i]].y = estCoord.at(i,1);

                        //                qDebug("a%d = [%f,%f]", i, _ancArray[i].x, _ancArray[i].y);

                        //emit anchPos(reveiveAID[i], _ancArray[reveiveAID[i]].x, _ancArray[reveiveAID[i]].y, _ancArray[reveiveAID[i]].z, false, true); // Update table entry
                    }
                    isFirst = false;
                    myGraphicsWidget->calibrationSuccess();
                    errNum = 0;
                }
                else
                {
                    //有效数据缺失，需提醒标定失败
                    errNum++;
                    myGraphicsWidget->calibrationSuccess();//错误数<3: 抛弃本次数据，开启下一次自标定，无弹出提示
                    if(errNum >= MAX_ERR_NUM)
                    {
                        QString err_aid = "";
                        for (int i = 0; i < MAX_NUM_ANCS; i++)
                        {
                            //if(stationList[i] == true)
                            {
                                for (int j = 0; j < MAX_NUM_ANCS; j++)
                                {
                                    if(matrixBuff[i][j] != 0)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        if(j == MAX_NUM_ANCS-1)
                                        {
                                            err_aid += ("A"+QString::number(i)+" ");
                                        }
                                    }
                                }
                            }
                        }
                        myGraphicsWidget->calibrationFailed();
                        if(language)
                        {
                            QMessageBox::warning(NULL,"警告","自标定失败\n请检查"+err_aid+"基站是否在线","我知道了");
                        }
                        else{
                            QMessageBox::warning(NULL,"Warning","Self-calibration failure Please check whether the "+err_aid+" station is online.","OK");
                        }

                        errNum = 0;
                    }
                }

                //重置buff数据
                for (int i = 0; i < MAX_NUM_ANCS; i++)
                {
                    for (int j = 0; j < MAX_NUM_ANCS; j++)
                    {
                        matrixBuff[i][j] = 0;
                    }
                }
            }
            aid_last = aid;
        }
        return;
    }
    else
    {
        return;
    }

    int idx = processTagRangeReports(tid, range, lnum, seq, mask, rx_power); //this is received when tags range to anchors
    if(idx != -1)
        trilaterateTag(tid, seq, idx);


}

void RTLSClient::timeOutWarning()
{
    QString err_aid = "";
    for (int i = 0; i < MAX_NUM_ANCS; i++)
    {
        //if(stationList[i] == true)
        {
            for (int j = 0; j < MAX_NUM_ANCS; j++)
            {
                if(matrixBuff[i][j] != 0)
                {
                    break;
                }
                else
                {
                    if(j == MAX_NUM_ANCS-1)
                    {
                        err_aid += ("A"+QString::number(i)+" ");
                    }
                }
            }
        }
    }
    myGraphicsWidget->calibrationFailed();
    if(language)
    {
        QMessageBox::warning(NULL,"警告","自标定失败\n请检查"+err_aid+"基站是否在线","我知道了");
    }
    else{
        QMessageBox::warning(NULL,"Warning","Self-calibration failure Please check whether the "+err_aid+" station is online.","OK");
    }
}

// void RTLSClient::CheckCommFrame(const char* pbuf, int revLen)//串口有效数据校验
// {
//     QByteArray data;
//     data = QByteArray(pbuf, revLen);
    
//     qDebug("pbuf = %s\n", pbuf);
//     qDebug("data = %s\n", data);

//     QDateTime now = QDateTime::currentDateTime();
//     QString nowstr = now.toString("hh:mm:ss:zzz ");
//     int length = data.length();
//     int offset = 0;
//     QString statusMsg;
//     QByteArray tofReport = data.mid(offset, TOF_REPORT_LEN);

//     qDebug("tofReport.constData() = %s\n", tofReport.constData());


//     //mc ff ffffffff ffffffff 00002134 000005dc ffffffff ffffffff ffffffff ffffffff 095f c1 00024c24 a01:0
//     //mc ff 00001509 000024da 000024da 00001509 095f c1 00024c24 a01:0
//     int aid, tid, range[8], lnum, seq, mask;
//     int rangetime;
//     char c, type;
//     int rx_power;

//     for(int i=0; i<MAX_ANC_NUM; i++)
//     {
//         range[i]=-1;
//     }
//     rx_power=800;

//     if((length == 107) || (length == 106))//8基站，带rxpower(107字节)
//     {
//         int n = sscanf(tofReport.constData(),"m%c %x %x %x %x %x %x %x %x %x %x %x %x %c%d:%d %x", &type, &mask, &range[0], &range[1], &range[2], &range[3], &range[4], &range[5], &range[6], &range[7], &lnum, &seq, &rangetime, &c, &tid, &aid, &rx_power);
//     }
//     else if((length == 71) || (length == 70)) //4基站，带rxpower(71字节)
//     {
//         int n = sscanf(tofReport.constData(),"m%c %x %x %x %x %x %x %x %x %c%d:%d %x", &type, &mask, &range[0], &range[1], &range[2], &range[3], &lnum, &seq, &rangetime, &c, &tid, &aid, &rx_power);
//     }
//     else if((length == 66)||(length == 65)) //4基站，不带rxpower(66字节)
//     {
//         int n = sscanf(tofReport.constData(),"m%c %x %x %x %x %x %x %x %x %c%d:%d", &type, &mask, &range[0], &range[1], &range[2], &range[3], &lnum, &seq, &rangetime, &c, &tid, &aid);
//     }
//     else if((length == 102)||(length == 101))//8基站，不带rxpower(102字节)
//     {
//         int n = sscanf(tofReport.constData(),"m%c %x %x %x %x %x %x %x %x %x %x %x %x %c%d:%d", &type, &mask, &range[0], &range[1], &range[2], &range[3], &range[4], &range[5], &range[6], &range[7], &lnum, &seq, &rangetime, &c, &tid, &aid);
//     }
//     else
//     {
//         //qDebug() << "err lengh="<< length;
//     }

//     //notify the user if connected to a tag or an anchor
//     if(_first)
//     {
//         if(c == 'a') //we are connected to an anchor id = i
//         {
//             statusMsg = "Connected to Anchor ID" + QString(" %1.").arg(aid);

//             emit statusBarMessage(statusMsg);
//         }
//         else
//         if(c == 't') //we are connected to a tag id = i
//         {
//             statusMsg = "Connected to Tag ID" + QString(" %1.").arg(tid);

//             emit statusBarMessage(statusMsg);
//         }
//         else
//         if(c == 'l') //we are connected to a listener id = i
//         {
//             statusMsg = "Connected to Listener ID" + QString(" %1.").arg(aid);

//             emit statusBarMessage(statusMsg);
//         }
//         //log the anchor co-ordinates to the file
//         for(int j=0; j<MAX_NUM_ANCS; j++)
//         {
//             if(_file)
//             {
//                 QString s =  nowstr + QString("Anchor%1(%2,%3,%4)\n").arg(j).arg(_ancArray[j].x).arg(_ancArray[j].y).arg(_ancArray[j].z);
//                 QTextStream ts( _file );
//                 ts << s;
//             }
//         }
//         _first = false;
//     }

//     if(type == 'c') //if 'c' these reports relate to tag <-> anchor ranges
//     {
//         //qDebug("rx_power111111 = %x",rx_power);
//         int idx = processTagRangeReports(tid, range, lnum, seq, mask, rx_power); //this is received when tags range to anchors

//         if(idx != -1)
//             trilaterateTag(tid, seq, idx);

//     }

// }


//calculate average (of last 50) excluding min and max
double RTLSClient::process_avg(int idx)
{
    double max, min, sum;
    int i;

    sum = max = min = _ancRangeArray[idx][0];

    for(i=0; i<ANC_RANGE_HIST; i++)
    {
        if (_ancRangeArray[idx][i] > max) max = _ancRangeArray[idx][i];
        if (_ancRangeArray[idx][i] < min) min = _ancRangeArray[idx][i];
        sum += _ancRangeArray[idx][i];
    }

    sum = sum - max - min;

    return sum / (ANC_RANGE_HIST - 2);
}

void RTLSClient::processAnchRangeReport(int aid, int tid, int range, int lnum, int seq)
{
    QDateTime now = QDateTime::currentDateTime();
    QString nowstr = ("hh:mm:ss:zzz ");
    //qDebug() << "a and t " << aid << tid << "correction = " << (_ancArray[aid].tagRangeCorection[tid] * 0.01);

    //log data to file
    // if(_file)
    // {
    //     QString s =  nowstr + QString("RA:%1:%2:%3:%4:%5:%6\n").arg(tid).arg(aid).arg(range).arg(0).arg(seq).arg(lnum);
    //     QTextStream ts( _file );
    //     ts << s;
    // }

    //find the anchor in the list (A0 to A1, is the same as A1 to A0)
    _ancRangeValues[aid][tid] = ((double)range) / 1000;
    _ancRangeValues[tid][aid] = _ancRangeValues[aid][tid];
    if(seq != _ancRangeLastSeq) //new ranges - send signal to GUI
    {
        //first we store the 50 ranges and find and average range, then we calculate position
        if(_ancRangeCount < ANC_RANGE_HIST)
        {
            _ancRangeArray[0][_ancRangeCount] = _ancRangeValues[0][1]; //range A0-A1
            _ancRangeArray[1][_ancRangeCount] = _ancRangeValues[0][2]; //range A0-A2
            _ancRangeArray[2][_ancRangeCount] = _ancRangeValues[1][2]; //rnage A1-A2
            _ancRangeCount++;
        }
        else //calculate average and then location
        {
            _ancRangeCount = 0;
            qInfo()<<_useAutoPos<<aid<<tid<<range<<lnum<<seq;
            if(_useAutoPos) //if Anchor auto positioning is enabled then process Anchor-Anchor TWR data
            {
                //we have 3 ranges (A0 to A1, A0 to A2 and A1 to A2)
                //calculate A0, A1 and A2 coordinates (x, y) based on A0 = 0,0 and assume A2 is to the right of A1

                int nNodes = 3;
                int viewNode = 0;
                mat twrdistance = zeros<mat>(nNodes,nNodes);
                mat transCoord = zeros<mat>(nNodes,nDim);
                mat estCoord = zeros<mat>(nNodes,nDim);

                //calculate average (exclude min, max)
                _ancRangeValuesAvg[0][1] = _ancRangeValuesAvg[1][0] = process_avg(0);
                _ancRangeValuesAvg[0][2] = _ancRangeValuesAvg[2][0] = process_avg(1);
                _ancRangeValuesAvg[1][2] = _ancRangeValuesAvg[2][1] = process_avg(2);

                for (int i = 0; i<nNodes; i++)                  // change the array to mat format
                {
                    for (int j = 0; j<nNodes; j++)
                    {
                       twrdistance(i,j) =  _ancRangeValuesAvg[i][j];
                    }
                }

                mds(twrdistance, nNodes, viewNode, &transCoord); // Using multi-dimension scaling to estimation the shape

                angleRotation(transCoord, nNodes, &estCoord); // estCoord is the coordinates with a1 on the x axis and a2 in the first quadrant

                //emit ancRanges(_ancRangeValues[0][1], _ancRangeValues[0][2], _ancRangeValues[1][2]); //send the update to graphic
                for(int i=0; i<(MAX_NUM_ANCS-1); i++) // Only update positions of A0, A1 and A2
                {
                    //_ancArray[i].x = estCoord.at(i,0);
                    //_ancArray[i].y = estCoord.at(i,1);

                    qInfo()<<i<<_ancArray[i].x<<_ancArray[i].y<<_ancArray[i].z;
                    //emit anchPos(i, _ancArray[i].x, _ancArray[i].y, _ancArray[i].z, false, true); // Update table entry
                }

                //if()
                //{
                   // emit centerOnAnchors(); - don't auto center ...
                //}
            }
        }
    }

    _ancRangeLastSeq = seq;
}





void RTLSClient::mds(mat twrdistance, int nNodes, int viewNode, mat* transCoord)
{
    mat         distSquared = zeros<mat>(nNodes,nNodes);
    mat         u = zeros<mat>(nNodes,nNodes);
    mat         v = zeros<mat>(nNodes,nNodes);
    colvec      s = zeros<vec>(nNodes);
    mat         estGeom = zeros<mat>(nNodes,nDim);

    distSquared = square(twrdistance);

    mat centeringOperator = eye<mat>(nNodes,nNodes)-(ones<mat>(nNodes,nNodes)/nNodes);

    mat centeredDistSquared = -0.5 * centeringOperator * distSquared * centeringOperator;

    svd(u,s,v,centeredDistSquared);

    for (int i = 0; i<nDim; i++)
    {
        estGeom.col(i) = u.col(i)*sqrt(s(i));
    }

    for (int i = 0; i<nNodes; i++)
    {
        transCoord->row(i) = estGeom.row(i)-estGeom.row(viewNode);
    }


}

void RTLSClient::angleRotation(mat transCoord, int nNodes, mat* estCoord)
{
    mat rotationMatrix = -ones<mat>(2,2);
    colvec transdistance = zeros(nNodes,1);
    mat Coord = zeros<mat>(nNodes,nDim);


    for (int i = 0; i<nNodes; i++)
    {
        transdistance.at(i) = sqrt(transCoord(i,0)*transCoord(i,0) + transCoord(i,1)*transCoord(i,1));

    }


    colvec currentAngle = acos(transCoord.col(0)/transdistance);

    for (int i = 0; i<nNodes; i++)
    {
        if (transCoord(i,1)<0)
        {
            currentAngle(i) = -currentAngle(i);
        }
    }

    double rotateAngle = currentAngle(1);

    rotationMatrix << cos(rotateAngle) << (-sin(rotateAngle)) << endr
                   << sin(rotateAngle) << cos(rotateAngle) <<endr;

    Coord = transCoord*rotationMatrix;

    if (Coord.at(2,1)<0)
    {
        Coord.at(2,1) = -Coord.at(2,1);
    }

    *estCoord = Coord;

}



int RTLSClient::processTagRangeReports(int tid, int *range, int lnum, int seq, int mask, int rx_power)
{
    int range_corrected = 0;
    int idx = 0;
    int seq_i ;
    int tag_index = -1;
    uint8_t seq_diff = 0;
    QTextStream ts (_file);
    //qDebug("rx_power222222222222 = %d",rx_power);

    QDateTime now = QDateTime::currentDateTime();
    QString nowstr = now.toString("hh:mm:ss:zzz ");
//    qDebug() << "a and t " << aid << tid << "correction = " << (_ancArray[aid].tagRangeCorection[tid] * 0.01);

    //find the tag in the list
    for(idx=0; idx<_tagList.size(); idx++)
    {
        //find this tag in the list
        if(_tagList.at(idx).id == tid)
        {
            tag_index = idx;
            break;
        }
    }

    //if we don't have this tag in the list add it
    if(idx == _tagList.size())
    {
        initialiseTagList(tid);
    }

    tag_reports_t rp = _tagList.at(idx);

    seq_i =  seq & 0xFF;

    if(rp.rangeSeq != -1)
    {
        seq_diff = (seq_i - rp.rangeSeq) & 0xFF;

        rp.printStats += seq_diff;

        //qDebug() << rp.printStats;
    }
    else
    {
        rp.printStats = 1;
    }
    rp.rangeCount[seq_i] = 0;
    rp.rangeSeq = seq_i;

    double rx_power_d;
    
    if(rx_power != 800)
    {
        rx_power_d = -((double)rx_power/100.0);
        //qDebug("rx_power = %f",rx_power_d);
    }
    else
    {
        rx_power_d=800;//错误代码
    }

    //qDebug("rx_powerd = %f",rx_power_d);

    //check the mask and process the tag - anchor ranges
    //for(int k=0; k<MAX_NUM_ANCS; k++)
    for(int k=0; k < 8; k++)
    {
        if((0x1 << k) & mask) //we have a valid range
        {
            range_corrected = range[k] + (_ancArray[k].tagRangeCorection[tid] * 10); //range correction is in cm (range is in mm)

            if(_file)
            {
                QString s =  nowstr + QString("TAG%1-ANC%2=%3\n").arg(tid).arg(k).arg(range_corrected);
                ts << s;
            }

            emit tagRange(tid, k, (range_corrected * 0.001), rx_power_d); //convert to meters

            RangeA0_A7[k]=range_corrected * 0.001;

            rp.rangeCount[seq_i]++;
            rp.rangeValue[seq_i][k] = range_corrected;

            //qDebug()<<"range_corrected= "<<range_corrected;

        }
        else
        {
            rp.rangeValue[seq_i][k] = 0;

            emit tagRange(tid, k, -1, -1); //report no/missing range
        }
    }

    //log data to file
    // if(_file)
    // {
    //     QString s =  nowstr + QString("RM:%1:%2:%3:%4\n").arg(tid).arg(mask).arg(seq).arg(lnum);
    //     ts << s;
    // }

    rp.rangeCountM[seq_i] = mask ;

    if(rp.printStats == 256) //print every 256 ranges
    {
        float a0r = 0;
        float a1r = 0;
        float a2r = 0;
        float a3r = 0;
        float a4r = 0;
        float a5r = 0;
        float a6r = 0;
        float a7r = 0;

        int missing = 0;

        for(int i=0; i<256; i++)
        {
            if(rp.rangeCountM[i] & 0x01) {a0r++;}
            if(rp.rangeCountM[i] & 0x02) {a1r++;}
            if(rp.rangeCountM[i] & 0x04) {a2r++;}
            if(rp.rangeCountM[i] & 0x08) {a3r++;}
            if(rp.rangeCountM[i] & 0x10) {a4r++;}
            if(rp.rangeCountM[i] & 0x20) {a5r++;}
            if(rp.rangeCountM[i] & 0x40) {a6r++;}
            if(rp.rangeCountM[i] & 0x80) {a7r++;}

            if(rp.rangeCountM[i] == 0) {missing++;}
            rp.rangeCountM[i] = 0 ; //clear all masks/data
        }

        a0r *= 100.0/256;
        a1r *= 100.0/256;
        a2r *= 100.0/256;
        a3r *= 100.0/256;
        a4r *= 100.0/256;
        a5r *= 100.0/256;
        a6r *= 100.0/256;
        a7r *= 100.0/256;

        //log data to file
        // if(_file)
        // {
        //     QString s =  nowstr +
        //             QString("RS:%1:%2:%3:%4:%5:%6:%7:%8:%9:%10\n").
        //             arg(tid).
        //             arg(QString::number(a0r, 'f', 2)).
        //             arg(QString::number(a1r, 'f', 2)).
        //             arg(QString::number(a2r, 'f', 2)).
        //             arg(QString::number(a3r, 'f', 2)).
        //             arg(QString::number(a4r, 'f', 2)).
        //             arg(QString::number(a5r, 'f', 2)).
        //             arg(QString::number(a6r, 'f', 2)).
        //             arg(QString::number(a7r, 'f', 2)).
        //             arg(missing);
        //     ts << s;
        // }

        rp.printStats = 0;
    }

    //update the list entry
    _tagList.replace(idx, rp);

    return tag_index;
}

void RTLSClient::trilaterateTagG(int tid, int seq, int idx, int *indexArray)
{
    int count = 0;
    //bool trilaterate = false;
    vec3d report;
    bool newposition = false;
    int nolocation = 0;
    int lastSeq = 0;
    QDateTime now = QDateTime::currentDateTime();
    QString nowstr = now.toString("hh:mm:ss:zzz ");

    tag_reports_t rp = _tagList.at(idx);

    //lastSeq = (seq-1) & 0xFF;
    lastSeq = seq;
    count = rp.rangeCount[lastSeq];

//    qDebug() << "count=" << count<<", stationNum: "<<stationNum;//几个基站数
//    qDebug() << "rang0=" << rp.rangeValue[lastSeq][0];
//    qDebug() << "rang1=" << rp.rangeValue[lastSeq][1];
//    qDebug() << "rang2=" << rp.rangeValue[lastSeq][2];
//    qDebug() << "rang3=" << rp.rangeValue[lastSeq][3];
//    qDebug() << "rang4=" << rp.rangeValue[lastSeq][4];
//    qDebug() << "rang5=" << rp.rangeValue[lastSeq][5];
//    qDebug() << "rang6=" << rp.rangeValue[lastSeq][6];
//    qDebug() << "rang7=" << rp.rangeValue[lastSeq][7];


    //we got next range seq. lets try and trilaterate the previous
    if(((stationNumber >= 3) && (checkbox2d3d >= 2)) || ((stationNumber >= 2) && (checkbox2d3d == 1)))
    {
        //qDebug() << "try to get location";

        if(calculateTagLocationG(&report, &rp.rangeValue[lastSeq][0], indexArray) > 0)
        {
            newposition = true;
            rp.numberOfLEs++;            

            mSocket = new QUdpSocket(this);
            mSocket->bind(QHostAddress::Any, 8550);
            
            if(BTN_sendSocketText=="发送"||BTN_sendSocketText=="Send")
            {

            }
            else
            {
                //QString ss = nowstr + QString("TAG%1,%2,%3,%4\n").arg(tid).arg(report.x).arg(report.y).arg(report.z);

                m_json.insert("Z", QString::number(rp.fz, 'f', 3).toDouble());
                m_json.insert("Y", QString::number(rp.fy, 'f', 3).toDouble());   
                m_json.insert("X", QString::number(rp.fx, 'f', 3).toDouble());
                m_json.insert("TagID", (tid));
                m_json.insert("Command", "UpLink");

                QJsonDocument _jsonDoc(m_json);
                QByteArray _byteArray = _jsonDoc.toJson(QJsonDocument::Indented);

                QString IP = LE_ip_addressText;
                int comID = LE_comTexttoInt;
                
                mSocket->writeDatagram(_byteArray, QHostAddress(IP), comID);
                
                mSocket->disconnectFromHost();
                mSocket->close();
                if(mSocket != NULL)
                {
                    delete mSocket;
                }
            }
            
            if(_file)
            {
                //log data to file
                //QString s = nowstr + QString("TAG%1[%2,%3,%4]\n").arg(id).arg(rp.av_x,0,'f',3).arg(rp.av_y,0,'f',3).arg(rp.av_z,0,'f',3);
                QString s;
                if((rp.fx!=0) && (rp.fy!=0) && (rp.fz!=0))
                    s = nowstr + QString("TAG%1[%2,%3,%4]\n").arg(tid).arg(rp.fx,0,'f',3).arg(rp.fy,0,'f',3).arg(rp.fz,0,'f',3);
                else
                    s = nowstr + QString("TAG%1[%2,%3,%4]\n").arg(tid).arg(report.x,0,'f',3).arg(report.y,0,'f',3).arg(report.z,0,'f',3);
                QTextStream ts( _file );
                ts << s;
            }

            // if(_file)
            // {
            //     // QString s = nowstr + QString("TAG%1[%2,%3,%4]\n").arg(tid).arg(report.x).arg(report.y).arg(report.z) +
            //     //         QString("A0=%1,A1=%2,A2=%3,A3=%4,A4=%5,A5=%6,A6=%7,A7=%8\n").arg(rp.rangeValue[lastSeq][0]).arg(rp.rangeValue[lastSeq][1]).arg(rp.rangeValue[lastSeq][2]).arg(rp.rangeValue[lastSeq][3]).arg(rp.rangeValue[lastSeq][4]).arg(rp.rangeValue[lastSeq][5]).arg(rp.rangeValue[lastSeq][6]).arg(rp.rangeValue[lastSeq][7]);
            //     QString s;
            //     if(_usingFilter == 0)//未使用滤波
            //     {
            //         s = nowstr + QString("TAG%1[%2,%3,%4]\n").arg(tid).arg(report.x,0,'f',3).arg(report.y,0,'f',3).arg(report.z,0,'f',3);
            //     }

            //     QTextStream ts( _file );
            //     ts << s;
            // }
            //qDebug() << "emit tagPos" << rp.numberOfLEs;
            if(_usingFilter == 0)
            {
                qDebug() << "emit tagPos tid:" << tid;
                emit tagPos(tid, report.x, report.y, report.z); //send the update to graphic
                emit tag3DPos(tid, report.x, report.y, report.z);
            }
            if(nolocation)
            {
                emit statusBarMessage("");
            }
            nolocation = 0;
        }
        else //no solution
        {
            //qDebug() << "nolocation";
            nolocation++;

            //log data to file
            if(_file)
            {
                QString s = nowstr + QString("TAG%1[nan,nan,nan]\n").arg(tid);
                QTextStream ts( _file );
                ts << s;
            }

            if( nolocation >= 5)
            {
                emit statusBarMessage("No location solution.");
            }
        }

    }
    //clear the count
    rp.rangeCount[lastSeq] = 0;

    //update the list entry
    _tagList.replace(idx, rp);

    //update statistics if new position has been calculated
    if(newposition)
    {
        updateTagStatistics(idx, report.x, report.y, report.z);

        if(_usingFilter != 0)
        {
            //if((rp.fx!=0)&&(rp.fy!=0)&&(rp.fz!=0))
            {
                emit tagPos(tid, rp.fx, rp.fy, rp.fz); //send the update to graphic
                emit tag3DPos(tid, rp.fx, rp.fy, rp.fz);

            }
        }
    }
    //qDebug() << "newposition" << newposition << idx << lastSeq << seq;
}


void RTLSClient::trilaterateTag(int tid, int seq, int idx)
{
    int count = 0;
    //bool trilaterate = false;
    vec3d report;
    bool newposition = false;
    int nolocation = 0;
    int lastSeq = 0;

    QDateTime now = QDateTime::currentDateTime();
    QString nowstr = now.toString("hh:mm:ss:zzz ");

    tag_reports_t rp = _tagList.at(idx);

    //lastSeq = (seq-1) & 0xFF;
    lastSeq = seq;
    count = rp.rangeCount[lastSeq];

//    qDebug() << "count=" << count<<", stationNum: "<<stationNum;//几个基站数
//    qDebug() << "rang0=" << rp.rangeValue[lastSeq][0];
//    qDebug() << "rang1=" << rp.rangeValue[lastSeq][1];
//    qDebug() << "rang2=" << rp.rangeValue[lastSeq][2];
//    qDebug() << "rang3=" << rp.rangeValue[lastSeq][3];
//    qDebug() << "rang4=" << rp.rangeValue[lastSeq][4];
//    qDebug() << "rang5=" << rp.rangeValue[lastSeq][5];
//    qDebug() << "rang6=" << rp.rangeValue[lastSeq][6];
//    qDebug() << "rang7=" << rp.rangeValue[lastSeq][7];


    //we got next range seq. lets try and trilaterate the previous
    if(((stationNumber >= 3) && (checkbox2d3d >= 2)) || ((stationNumber >= 2) && (checkbox2d3d == 1)))
    {
        //qDebug() << "try to get location";

        if(calculateTagLocation(&report, &rp.rangeValue[lastSeq][0] ) > 0)
        {
            newposition = true;
            rp.numberOfLEs++;            

            mSocket = new QUdpSocket(this);
            mSocket->bind(QHostAddress::Any, 8550);
            
            if(BTN_sendSocketText=="发送"||BTN_sendSocketText=="Send")
            {

            }
            else
            {
                //QString ss = nowstr + QString("TAG%1,%2,%3,%4\n").arg(tid).arg(report.x).arg(report.y).arg(report.z);
                m_json.insert("Z", QString::number(rp.fz, 'f', 3).toDouble());
                m_json.insert("Y", QString::number(rp.fy, 'f', 3).toDouble());   
                m_json.insert("X", QString::number(rp.fx, 'f', 3).toDouble());
                m_json.insert("TagID", (tid));
                m_json.insert("Command", "UpLink");

                QJsonDocument _jsonDoc(m_json);
                QByteArray _byteArray = _jsonDoc.toJson(QJsonDocument::Indented);

                QString IP = LE_ip_addressText;
                int comID = LE_comTexttoInt;
                
                mSocket->writeDatagram(_byteArray, QHostAddress(IP), comID);
                
                mSocket->disconnectFromHost();
                mSocket->close();
                if(mSocket != NULL)
                {
                    delete mSocket;
                }
            }
            if(_file)
            {
                //log data to file
                //QString s = nowstr + QString("TAG%1[%2,%3,%4]\n").arg(id).arg(rp.av_x,0,'f',3).arg(rp.av_y,0,'f',3).arg(rp.av_z,0,'f',3);
                QString s;
                if((rp.fx!=0) && (rp.fy!=0) && (rp.fz!=0))
                    s = nowstr + QString("TAG%1[%2,%3,%4]\n").arg(tid).arg(rp.fx,0,'f',3).arg(rp.fy,0,'f',3).arg(rp.fz,0,'f',3);
                else
                    s = nowstr + QString("TAG%1[%2,%3,%4]\n").arg(tid).arg(report.x,0,'f',3).arg(report.y,0,'f',3).arg(report.z,0,'f',3);
                QTextStream ts( _file );
                ts << s;
            }


            // if(_file)
            // {
            //     // QString s = nowstr + QString("TAG%1[%2,%3,%4]\n").arg(tid).arg(report.x).arg(report.y).arg(report.z) +
            //     //         QString("A0=%1,A1=%2,A2=%3,A3=%4,A4=%5,A5=%6,A6=%7,A7=%8\n").arg(rp.rangeValue[lastSeq][0]).arg(rp.rangeValue[lastSeq][1]).arg(rp.rangeValue[lastSeq][2]).arg(rp.rangeValue[lastSeq][3]).arg(rp.rangeValue[lastSeq][4]).arg(rp.rangeValue[lastSeq][5]).arg(rp.rangeValue[lastSeq][6]).arg(rp.rangeValue[lastSeq][7]);
            //     QString s;
            //     if(_usingFilter == 0)//未使用滤波
            //     {
            //         s = nowstr + QString("TAG%1[%2,%3,%4]\n").arg(tid).arg(report.x,0,'f',3).arg(report.y,0,'f',3).arg(report.z,0,'f',3);
            //     }
                
            //     QTextStream ts( _file );
            //     ts << s;
            // }
            //qDebug() << "emit tagPos" << rp.numberOfLEs;
            if(_usingFilter == 0)
            {
                qDebug() << "emit tagPos tid:" << tid;
                emit tagPos(tid, report.x, report.y, report.z); //send the update to graphic
                emit tag3DPos(tid, report.x, report.y, report.z);
            }
            if(nolocation)
            {
                emit statusBarMessage("");
            }
            nolocation = 0;
        }
        else //no solution
        {
            //qDebug() << "nolocation";
            nolocation++;

            //log data to file
            if(_file)
            {
                QString s = nowstr + QString("TAG%1[nan,nan,nan]\n").arg(tid);
                QTextStream ts( _file );
                ts << s;
            }

            if(nolocation >= 5)
            {
                emit statusBarMessage("No location solution.");
            }
        }

    }
    //clear the count
    rp.rangeCount[lastSeq] = 0;

    //update the list entry
    _tagList.replace(idx, rp);

    //update statistics if new position has been calculated
    if(newposition)
    {
        updateTagStatistics(idx, report.x, report.y, report.z);

        if(_usingFilter != 0)
        {
            //if((rp.fx!=0)&&(rp.fy!=0)&&(rp.fz!=0))
            {
                emit tagPos(tid, rp.fx, rp.fy, rp.fz); //send the update to graphic
                emit tag3DPos(tid, rp.fx, rp.fy, rp.fz);

            }
        }
    }
    //qDebug() << "newposition" << newposition << idx << lastSeq << seq;
}

int RTLSClient::calculateTagLocationG(vec3d *report, int *ranges, int *indexArray)
{
    int result = 0;
    vec3d anchorArray[8];

    anchorArray[0].x = _ancArray[indexArray[0]].x;
    anchorArray[0].y = _ancArray[indexArray[0]].y;
    anchorArray[0].z = _ancArray[indexArray[0]].z;

    anchorArray[1].x = _ancArray[indexArray[1]].x;
    anchorArray[1].y = _ancArray[indexArray[1]].y;
    anchorArray[1].z = _ancArray[indexArray[1]].z;

    anchorArray[2].x = _ancArray[indexArray[2]].x;
    anchorArray[2].y = _ancArray[indexArray[2]].y;
    anchorArray[2].z = _ancArray[indexArray[2]].z;

    anchorArray[3].x = _ancArray[indexArray[3]].x;
    anchorArray[3].y = _ancArray[indexArray[3]].y;
    anchorArray[3].z = _ancArray[indexArray[3]].z;

    anchorArray[4].x = _ancArray[indexArray[4]].x;
    anchorArray[4].y = _ancArray[indexArray[4]].y;
    anchorArray[4].z = _ancArray[indexArray[4]].z;

    anchorArray[5].x = _ancArray[indexArray[5]].x;
    anchorArray[5].y = _ancArray[indexArray[5]].y;
    anchorArray[5].z = _ancArray[indexArray[5]].z;

    anchorArray[6].x = _ancArray[indexArray[6]].x;
    anchorArray[6].y = _ancArray[indexArray[6]].y;
    anchorArray[6].z = _ancArray[indexArray[6]].z;

    anchorArray[7].x = _ancArray[indexArray[7]].x;
    anchorArray[7].y = _ancArray[indexArray[7]].y;
    anchorArray[7].z = _ancArray[indexArray[7]].z;

    result = GetLocation(report, &anchorArray[0], ranges, checkbox2d3d);

    // if(checkbox2d3d == 2) //二维
    // {
    //    result = GetLocation(report, &anchorArray[0], ranges, 2);
    //    //qDebug("2222=%d", checkbox2d3d);
    //    //_usingFilter = 2;
    // }
    // else if(checkbox2d3d == 1) //一维
    // {
    //     result = GetLocation(report, &anchorArray[0], ranges, 1);
    //     //qDebug("1111=%d", checkbox2d3d);
    //     //_usingFilter = 0;
    //     // qDebug("result = %d\r\n",result);
	//     // qDebug("tag.x=%.3f\r\ntag.y=%.3f\r\ntag.z=%.3f\r\n", report->x, report->y, report->z);
    // }



    return result;
}

int RTLSClient::calculateTagLocation(vec3d *report, int *ranges)
{
    int result = 0;
    vec3d anchorArray[8];

    anchorArray[0].x = _ancArray[0].x;
    anchorArray[0].y = _ancArray[0].y;
    anchorArray[0].z = _ancArray[0].z;

    anchorArray[1].x = _ancArray[1].x;
    anchorArray[1].y = _ancArray[1].y;
    anchorArray[1].z = _ancArray[1].z;

    anchorArray[2].x = _ancArray[2].x;
    anchorArray[2].y = _ancArray[2].y;
    anchorArray[2].z = _ancArray[2].z;

    anchorArray[3].x = _ancArray[3].x;
    anchorArray[3].y = _ancArray[3].y;
    anchorArray[3].z = _ancArray[3].z;

    anchorArray[4].x = _ancArray[4].x;
    anchorArray[4].y = _ancArray[4].y;
    anchorArray[4].z = _ancArray[4].z;

    anchorArray[5].x = _ancArray[5].x;
    anchorArray[5].y = _ancArray[5].y;
    anchorArray[5].z = _ancArray[5].z;

    anchorArray[6].x = _ancArray[6].x;
    anchorArray[6].y = _ancArray[6].y;
    anchorArray[6].z = _ancArray[6].z;

    anchorArray[7].x = _ancArray[7].x;
    anchorArray[7].y = _ancArray[7].y;
    anchorArray[7].z = _ancArray[7].z;

    result = GetLocation(report, &anchorArray[0], ranges, checkbox2d3d);

    // if(checkbox2d3d == 2) //二维
    // {
    //    result = GetLocation(report, &anchorArray[0], ranges, 2);
    //    //qDebug("2222=%d", checkbox2d3d);
    //    //_usingFilter = 2;
    // }
    // else if(checkbox2d3d == 1) //一维
    // {
    //     result = GetLocation(report, &anchorArray[0], ranges, 1);
    //     //qDebug("1111=%d", checkbox2d3d);
    //     //_usingFilter = 0;
    //     // qDebug("result = %d\r\n",result);
	//     // qDebug("tag.x=%.3f\r\ntag.y=%.3f\r\ntag.z=%.3f\r\n", report->x, report->y, report->z);
    // }



    return result;
}

void RTLSClient::setGWReady(bool set)
{
    _graphicsWidgetReady = set;
}

void r95Sort (double s[], int l, int r)
{
    int i,j;
    double x;
    if(l<r)
    {
        i = l;
        j = r;
        x = s[i];
        while(i<j)
        {
            while (i<j&&s[j]>x) j--;
            if (i<j) s[i++] = s[j];
            while (i<j&&s[i]<x) i++;
            if (i < j) s[j--] = s[i];
        }
        s[i] = x;
        r95Sort(s, l, i-1);
        r95Sort(s, i+1, r);
    }

}

void RTLSClient::connectionStateChanged(SerialConnection::ConnectionState state)
{
   qDebug() << "RTLSClient::connectionStateChanged " << state;

    if(state == SerialConnection::Disconnected) //disconnect from Serial Port
    {
        if(_serial != NULL)
        {
            disconnect(_serial, SIGNAL(readyRead()), this, SLOT(newData()));
            _serial = NULL;
        }
        if(_file)
        {
            _file->close(); //close the Log file
        }
    }

}

void RTLSClient::addMissingAnchors(void)
{
    int i;

    //check how many anchors were added and add placeholders for any missing ones
    for(i=0; i<MAX_NUM_ANCS; i++)
    {
        if(_ancArray[i].id == 0xff)
        {
            _ancArray[i].id = i;
            emit anchPos(i, _ancArray[i].x, _ancArray[i].y, _ancArray[i].z, false, false);
        }
    }

    emit centerOnAnchors();
}

void RTLSClient::loadConfigFile(QString filename)
{
    QFile file(filename);

    double x[8] = {0.0, 5.0, 0.0, 5.0, 10.0, 10.0, 15.0, 15.0};
    double y[8] = {0.0, 0.0, 5.0, 5.0,  0.0,  5.0,  0.0,  5.0};

    for(int i=0; i < MAX_ANC_NUM; i++)
    {
        _ancArray[i].id = 0xff;
        _ancArray[i].label = "";
        _ancArray[i].x = x[i];  //default x
        _ancArray[i].y = y[i];  //default y
        _ancArray[i].z = 3.00;  //default z

        for(int j = 0; j<MAX_NUM_TAGS; j++)
        {
            _ancArray[i].tagRangeCorection[j] =  0;
        }
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg(filename).arg(file.errorString())));
        addMissingAnchors();
        return;
    }

    QDomDocument doc;
    QString error;
    int errorLine;
    int errorColumn;

    if(doc.setContent(&file, false, &error, &errorLine, &errorColumn))
    {
       // qDebug() << "1file error !!!" << error << errorLine << errorColumn;
    }

    QDomElement config = doc.documentElement();

    if( config.tagName() == "config" )
    {
        QDomNode n = config.firstChild();
        while( !n.isNull() )
        {
            QDomElement e = n.toElement();
            if( !e.isNull() )
            {
                if( e.tagName() == "anc" )
                {
                    bool ok;
                    int id = (e.attribute( "ID", "" )).toInt(&ok);

                    //id &= 0x3;

                    if(ok)
                    {
                        //_ancArray[id].id = id & 0xf;
                        _ancArray[id].label = (e.attribute( "label", "" ));
                        _ancArray[id].x = (e.attribute("x", "0.0")).toDouble(&ok);
                        _ancArray[id].y = (e.attribute("y", "0.0")).toDouble(&ok);
                        _ancArray[id].z = (e.attribute("z", "0.0")).toDouble(&ok);

                        //tag distance correction (in cm)
                        _ancArray[id].tagRangeCorection[0] = (e.attribute("t0", "0")).toDouble(&ok);
                        _ancArray[id].tagRangeCorection[1] = (e.attribute("t1", "0")).toDouble(&ok);
                        _ancArray[id].tagRangeCorection[2] = (e.attribute("t2", "0")).toDouble(&ok);
                        _ancArray[id].tagRangeCorection[3] = (e.attribute("t3", "0")).toDouble(&ok);
                        _ancArray[id].tagRangeCorection[4] = (e.attribute("t4", "0")).toDouble(&ok);
                        _ancArray[id].tagRangeCorection[5] = (e.attribute("t5", "0")).toDouble(&ok);
                        _ancArray[id].tagRangeCorection[6] = (e.attribute("t6", "0")).toDouble(&ok);
                        _ancArray[id].tagRangeCorection[7] = (e.attribute("t7", "0")).toDouble(&ok);



                        if(id >= 3) //hide anchor 4-8 by default
                        {
                            emit anchPos(id, _ancArray[id].x, _ancArray[id].y, _ancArray[id].z, false, false);
                        }
                        else
                        {
                            //emit anchPos(id, _ancArray[id].x, _ancArray[id].y, _ancArray[id].z, true, false);
                            emit anchPos(id, _ancArray[id].x, _ancArray[id].y, _ancArray[id].z, false, false);
                        }
                    }
                }
            }

            n = n.nextSibling();
        }

    }

    file.close();

    addMissingAnchors();
}

// 获取文件的绝对路径
QString getAbsolutePath(const QString &fileName)
{
    QFile file(fileName);
    return QFileInfo(file).absoluteFilePath();
}

void RTLSClient::saveBaseStation(const QString &fileName)
{
    qDebug() << "Into saveBaseStation";
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug() << "Could not open file for writing";
        return;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("BaseStations");

    for (const BaseStation &station : baseStationList)
    {
        xmlWriter.writeStartElement("BaseStation");

        xmlWriter.writeTextElement("GroupId", QString::number(station.groupId));
        xmlWriter.writeTextElement("AnchorId", QString::number(station.anchorId));
        //xmlWriter.writeTextElement("Status", station.status ? "true" : "false");
        xmlWriter.writeTextElement("Status", "false");
        xmlWriter.writeTextElement("X", QString::number(station.x));
        xmlWriter.writeTextElement("Y", QString::number(station.y));
        xmlWriter.writeTextElement("Z", QString::number(station.z));
        xmlWriter.writeTextElement("IP", station.ip);
        xmlWriter.writeTextElement("MAC", station.mac);
        xmlWriter.writeTextElement("AgingTime", QString::number(station.aging_time));
        xmlWriter.writeTextElement("SetGroupId", QString::number(station.set_groupId));

        xmlWriter.writeEndElement(); // BaseStation
    }

    xmlWriter.writeEndElement(); // BaseStations
    xmlWriter.writeEndDocument();
    file.close();

    //qDebug() << "Writing to file:" << fileName;
    QString absolutePath = getAbsolutePath(fileName);
    qDebug() << "Absolute Path of" << fileName << ":" << absolutePath;
    qDebug() << "Number of BaseStations:" << baseStationList.size();
}

void RTLSClient::loadBaseStation(const QString &fileName)
{
    int index = 0;
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Could not open file for reading";
        return;
    }

    QXmlStreamReader xmlReader(&file);
    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        QXmlStreamReader::TokenType token = xmlReader.readNext();
        if (token == QXmlStreamReader::StartElement)
        {
            if (xmlReader.name() == "BaseStation")
            {
                BaseStation station;

                while (!(xmlReader.tokenType() == QXmlStreamReader::EndElement &&
                         xmlReader.name() == "BaseStation"))
                {
                    xmlReader.readNext();

                    if (xmlReader.tokenType() == QXmlStreamReader::StartElement)
                    {
                        QStringRef elementName = xmlReader.name();

                        if (elementName == "GroupId")
                            station.groupId = xmlReader.readElementText().toInt();
                        else if (elementName == "AnchorId")
                            station.anchorId = xmlReader.readElementText().toInt();
                        else if (elementName == "Status")
                            station.status = (xmlReader.readElementText().toLower() == "true");
                        else if (elementName == "X")
                        {
                            station.x = xmlReader.readElementText().toDouble();
                             //_ancArray[index].x = station.x;
                        }
                        else if (elementName == "Y")
                        {
                            station.y = xmlReader.readElementText().toDouble();
                            //_ancArray[index].y = station.y;
                        }
                        else if (elementName == "Z")
                        {
                            station.z = xmlReader.readElementText().toDouble();
                            //_ancArray[index].z = station.z;
                        }
                        else if (elementName == "IP")
                            station.ip = xmlReader.readElementText();
                        else if (elementName == "MAC")
                            station.mac = xmlReader.readElementText();
                        else if (elementName == "AgingTime")
                            station.aging_time = xmlReader.readElementText().toInt();
                        else if (elementName == "SetGroupId")
                            station.set_groupId = xmlReader.readElementText().toInt();

                    }
                }
                qDebug() << "index:" << index;
                index++;
                loadStationList.append(station);

            }
        }
    }

    file.close();
}

QDomElement AnchorToNode( QDomDocument &d, anc_struct_t * anc )
{
    QDomElement cn = d.createElement( "anc" );
    cn.setAttribute("ID", QString::number(anc->id));
    cn.setAttribute("label", anc->label);
    cn.setAttribute("x", anc->x);
    cn.setAttribute("y", anc->y);
    cn.setAttribute("z", anc->z);
    cn.setAttribute("t0", anc->tagRangeCorection[0]);
    cn.setAttribute("t1", anc->tagRangeCorection[1]);
    cn.setAttribute("t2", anc->tagRangeCorection[2]);
    cn.setAttribute("t3", anc->tagRangeCorection[3]);
    cn.setAttribute("t4", anc->tagRangeCorection[4]);
    cn.setAttribute("t5", anc->tagRangeCorection[5]);
    cn.setAttribute("t6", anc->tagRangeCorection[6]);
    cn.setAttribute("t7", anc->tagRangeCorection[7]);

    return cn;
}

void RTLSClient::saveConfigFile(QString filename)
{
    QFile file( filename );
    int i = 0;

    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg(filename).arg(file.errorString())));
        return;
    }

    QDomDocument doc;

    // Adding tag config root
    QDomElement config = doc.createElement("config");
    doc.appendChild(config);

    while (i < MAX_NUM_ANCS)
    {
        config.appendChild( AnchorToNode(doc, &_ancArray[i]) );

        i++;
    }

    QTextStream ts( &file );
    ts << doc.toString();

    file.close();

    if(_file)
    {
        _file->flush();
    }

   // qDebug() << doc.toString();
}


void RTLSClient::updateAnchorXYZ(int id, int x, double value)
{
    QDateTime now = QDateTime::currentDateTime();
    QString nowstr = now.toString("hh:mm:ss:zzz ");

    qDebug() << "updateAnchorXYZ id:" << id << "x" << x << "value" << value;
    if(x == 3)//modify by jiajun 20231025
    {
        _ancArray[id].x = value;
        baseStationList[id].x = value;
    }
    else if (x == 4)
    {
        _ancArray[id].y = value;
        baseStationList[id].y = value;
    }
    else if (x == 5)
    {
        _ancArray[id].z = value;
        baseStationList[id].z = value;
    }
    //todo update 3d anchor
//    emit updateAnchorList3D(id,true,true,baseStationList[id].x,baseStationList[id].y,baseStationList[id].z);
    if(_file)
    {
        QString s =  nowstr + QString("Anchor%1(%2,%3,%4)\n").arg(id).arg(_ancArray[id].x).arg(_ancArray[id].y).arg(_ancArray[id].z);
        QTextStream ts( _file );
        ts << s;
    }
}


void RTLSClient::updateTagCorrection(int aid, int tid, int value)
{
    //tid &= 0x7;

    _ancArray[aid].tagRangeCorection[tid] = value;

}

int* RTLSClient::getTagCorrections(int anchID)
{
    return &_ancArray[anchID].tagRangeCorection[0];
}
//bool RTLSClient::event(QEvent *event)
//{
//    if(event->type() == QEvent::LanguageChange)
//    {
//        if(language)
//        {
//            _locationFilterTypes.clear();
//            _locationFilterTypes << "无" << "平均平滑滤波" << "去极值平滑滤波";
//            qDebug()<<"2222";

//        }
//        else{
//            _locationFilterTypes.clear();
//           _locationFilterTypes<< "null" << "Avg smoothing filter" << "De-extremum smoothing";
//           qDebug()<<"1111";

//        }
//    }
//   return QObject::event(event);
//}
