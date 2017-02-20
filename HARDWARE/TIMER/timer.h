#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"
#define GPSFailedTime_MAX 3000					//GPS��λʧ�ܳ�ʱʱ��  ����5����û�ж�λ�ɹ�
#define GPS_Switch_AGPS  30
#define Upload_Time_MAX  60*5      //����5����û���ϴ�
void TIM3_Int_Init(u16 arr,u16 psc);
void TIM7_Int_Init(u16 arr,u16 psc);
extern u32 GPS_Upload;   //GPS���ݴ����־λ
extern double LatNow,LonNow,LatOld,LonOld;//��¼�ϴ�ʱ��γ��
extern u8 Error;					//�жϳ����Ƿ���� 0:δ���� 1:����
extern u32 Upload_Time;   //�ϴ�������ʱ��
extern u32 Heartbeat_Upload;  //�����ϴ���־
extern u8 GPS_PACK;         //���δ����ɱ�־λ
extern u8 submit_info_flag;  //�ϴ���������־λ
extern u8 waitservice_flag;   //�ȴ���������ʱ��־
extern u8 LEDshine_flag;    
#endif
