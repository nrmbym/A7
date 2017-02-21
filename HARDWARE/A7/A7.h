#ifndef __A7_H
#define __A7_H
#include "sys.h"
#include "usart_2.h"
#include "usmat3.h"



#define Post_Errotimes_MAX 50   //最大允许出错次数

typedef struct
{
    vu16 USART2_RX_STA;			//接收到的数据状态//[15]:0,没有接收到数据;1,接收到了一批数据.//[14:0]:接收到的数据长度
    u8 Error;								//操作A6是否出错。0:未出错 1:出错
    u8 USART2_RX_BUF[USART2_MAX_RECV_LEN];//接收缓冲,最大USART2_MAX_RECV_LEN字节.
    u8 USART2_TX_BUF[USART2_MAX_SEND_LEN];//发送缓冲,最大USART2_MAX_SEND_LEN字节
    u8 signalQuality;			//GSM信号质量
    char revision[50];			//模块固件版本信息
    char IMEI[16];				//模块IMEI号
    char Operator[15];			//运营商
} Data_Pack;



extern char p1[300];
extern char POSTDATA[1000];
extern u8 failedTimes;     //上一次成功之后发送传输错误次数
extern u8 Post_Errotimes;   //A7一次开机最大次数
extern Data_Pack u2_data_Pack;  //A7相关结构体
extern u8 isSendDataError;  //发送错误标志位
extern u8 isSendData;     //通信标志位
extern u8 deviceState;    //A7工作状态标志
  

extern s8 sendAT(char *sendStr,char *searchStr,u32 outTime);//发送AT指令函数
extern s8 Check_TCP(void);    //检查当前连接状态
extern s8 GPRS_Connect(void);   //GPRS连接
extern s8 TCP_Connect(void);   //TCP连接
extern s8 HTTP_POST(char * GPS_Info); //POST发送
extern s8 extractParameter(char *sourceStr,char *searchStr,s32 *dataOne,s32 *dataTwo,s32 *dataThree);
extern s8 A7_GPRS_Test(void);  //检测GPRS连接状态

extern u8 charNum(char *p ,char Char);


extern void cleanReceiveData(void);    //清楚接收器
extern void checkRevision(void);//查询模块版本信息
extern void getIMEI(void); //获取IMEI
extern void A7_Init(void);   //初始化A7
extern void A7_GPS_Init(void);  //初始化A7
extern void A7_SendPost(void);  //发送并校验
extern void Restart_A7(void);   //硬件重启A7


extern char * my_strstr(char *searchStr);   //搜索字符串函数
//extern char *Analytica1Data_GET(void);//geT解析函数
extern char * AnalyticalData(void);//解析函数
extern char * ReturnCommaPosition(char *p ,u8 num);


#endif
