#ifndef __USMAT3_H
#define __USMAT3_H
#include "sys.h"

#define USART3_MAX_RECV_LEN		5000	//�����ջ����ֽ���
#define USART3_RX_EN 			1					//0,������;1,����.
#define SRTING_NUM 20
#define GPS_array 10        //GPS���ݴ洢������ 

typedef struct
{
    u16 year;    //��
    u8  month;   //��
    u8  day;     //��
    u8  hour;    //ʱ
    u8 minute;   //��
    u8 second;	  //��
    char Time[21];     //��ǰʱ��
    char Time_Old[21]; //��ʷʱ��
} Time_Pack;

__packed typedef struct     //__packed�������ʾ 1�������������Զ���ѹ������ �����ж���
{   //2��ʹ��δ����ķ��ʶ�ȡ��д��ѹ�����͵Ķ���
    s32 bootTimes;						//��������
    s32 submitInformationErrorTimes;//�ϴ�������ʧ�ܴ���
    s32 postErrorTimes;				//POSTʧ�ܴ���
    s32 GPSErrorTimes;				//��ΪGPSδ��λʱ�䳬ʱ��ɵ�A7��������
    u32 CRCData;					    //CRCData֮ǰ���ݵ�CRCУ��
    s32 isEffective;				  //��¼FLASH�е������Ƿ���Ч

} _sysData;

typedef union       //������,���湲��һ�δ洢�ռ�  һ��buf�д洢�����ֽ� ÿ����buf�洢һ������
{
    u16 buf[sizeof(_sysData)/2];//��ȡ��д��FLASH������
    _sysData data;	//�Զ���buf[]�е����ݷ���Ϊ��Ҫ������    buf��ÿ������Ӧһ������������
} _sysData_Pack;

extern u8 USART3_RX_BUF[USART3_MAX_RECV_LEN];   //�����ջ�����USART3_MAX_RECV_LEN�ֽ�
extern vu16 USART2_RX_STA;   						//��������״̬
extern char *GPRMC;		//������յ�������
void	USART3_Init(u32 BaudRate); 				//����3��ʼ��
extern void analysisGPS(void);     //GPS���ݽ�������
extern char* string[SRTING_NUM];

extern u16 receive_len;			//���ռ�����
extern u32 startNum;        //��¼��ǰ�����ַ����ڽ��ջ����еĿ�ʼλ��
extern char* string[SRTING_NUM];
extern char UTCTime[7];		       //UTCʱ�䣬hhmmss��ʽ
extern char Longitude_Str[11];	 //����dddmm.mmmm(�ȷ�)��ʽ
extern Time_Pack u2_time_Pack;		//ʱ��
extern double Latitude_Temp,Baidu_Latitude_Temp;    //��¼ת���ľ�γ��
extern double Longitude_Temp,Baidu_Longitude_Temp;  //��¼ת���ľ�γ��

extern double BaiduLongitude_Range[GPS_array];     //����(�ٶ�����)
extern double BaiduLatitude_Range[GPS_array];	     //γ��(�ٶ�����)
extern u8 GPS_effective;                           //GPS�����Ƿ���Чλ
extern void getFilterLoc(double * Lat_filter,double * Lon);   //GPS�����˲�����
extern void GPS_Packed_Data(void);    //GPS���ݴ���ϴ�����
extern void AGPS_Location(void);      //AGPS��ȡ��λ��Ϣ
extern _sysData_Pack sysData_Pack;
extern double distance;             //���������ϴ��ľ�����
extern void Receive_empty(void);    //���GPS����
extern u8 Pack_length;              //�������ݴ������
#endif
