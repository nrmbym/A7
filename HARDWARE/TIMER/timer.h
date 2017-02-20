#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"
#define GPSFailedTime_MAX 3000					//GPS定位失败超时时间  连续5分钟没有定位成功
#define GPS_Switch_AGPS  30
#define Upload_Time_MAX  60*5      //超过5分钟没有上传
void TIM3_Int_Init(u16 arr,u16 psc);
void TIM7_Int_Init(u16 arr,u16 psc);
extern u32 GPS_Upload;   //GPS数据打包标志位
extern double LatNow,LonNow,LatOld,LonOld;//记录上传时的纬度
extern u8 Error;					//判断程序是否出错 0:未出错 1:出错
extern u32 Upload_Time;   //上传服务器时间
extern u32 Heartbeat_Upload;  //心跳上传标志
extern u8 GPS_PACK;         //两次打包完成标志位
extern u8 submit_info_flag;  //上传服务器标志位
extern u8 waitservice_flag;   //等待服务器超时标志
extern u8 LEDshine_flag;    
#endif
