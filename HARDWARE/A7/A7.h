#ifndef __A7_H
#define __A7_H
#include "sys.h"
#include "usart_2.h"
#include "usmat3.h"



#define Post_Errotimes_MAX 50   //�������������

typedef struct
{
    vu16 USART2_RX_STA;			//���յ�������״̬//[15]:0,û�н��յ�����;1,���յ���һ������.//[14:0]:���յ������ݳ���
    u8 Error;								//����A6�Ƿ����0:δ���� 1:����
    u8 USART2_RX_BUF[USART2_MAX_RECV_LEN];//���ջ���,���USART2_MAX_RECV_LEN�ֽ�.
    u8 USART2_TX_BUF[USART2_MAX_SEND_LEN];//���ͻ���,���USART2_MAX_SEND_LEN�ֽ�
    u8 signalQuality;			//GSM�ź�����
    char revision[50];			//ģ��̼��汾��Ϣ
    char IMEI[16];				//ģ��IMEI��
    char Operator[15];			//��Ӫ��
} Data_Pack;



extern char p1[300];
extern char POSTDATA[1000];
extern u8 failedTimes;     //��һ�γɹ�֮���ʹ���������
extern u8 Post_Errotimes;   //A7һ�ο���������
extern Data_Pack u2_data_Pack;  //A7��ؽṹ��
extern u8 isSendDataError;  //���ʹ����־λ
extern u8 isSendData;     //ͨ�ű�־λ
extern u8 deviceState;    //A7����״̬��־
  

extern s8 sendAT(char *sendStr,char *searchStr,u32 outTime);//����ATָ���
extern s8 Check_TCP(void);    //��鵱ǰ����״̬
extern s8 GPRS_Connect(void);   //GPRS����
extern s8 TCP_Connect(void);   //TCP����
extern s8 HTTP_POST(char * GPS_Info); //POST����
extern s8 extractParameter(char *sourceStr,char *searchStr,s32 *dataOne,s32 *dataTwo,s32 *dataThree);
extern s8 A7_GPRS_Test(void);  //���GPRS����״̬

extern u8 charNum(char *p ,char Char);


extern void cleanReceiveData(void);    //���������
extern void checkRevision(void);//��ѯģ��汾��Ϣ
extern void getIMEI(void); //��ȡIMEI
extern void A7_Init(void);   //��ʼ��A7
extern void A7_GPS_Init(void);  //��ʼ��A7
extern void A7_SendPost(void);  //���Ͳ�У��
extern void Restart_A7(void);   //Ӳ������A7


extern char * my_strstr(char *searchStr);   //�����ַ�������
//extern char *Analytica1Data_GET(void);//geT��������
extern char * AnalyticalData(void);//��������
extern char * ReturnCommaPosition(char *p ,u8 num);


#endif
