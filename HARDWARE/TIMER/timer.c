#include "timer.h"
#include "usmat3.h"
#include "stdio.h"
#include "stmflash.h"
#include "iwdg.h"
#include "conversion.h"
#include "a7.h"
u32 GPS_Upload=0;    //GPS���ݴ����־
double 	LatNow = 0,LonNow = 0,LatOld=0,LonOld=0;//��¼�ϴ�ʱ��γ��
u32 GPS_invalidtimes=0;
u8 Error=0;					//����808�Ƿ����0:δ���� 1:����
u32 Upload_Time=0;  //�ϴ�������ʱ����
u32 Heartbeat_Upload=0;  //�����ϴ���־λ
u8 submit_info_flag=0;   //����GPS��־λ����
u8 GPS_PACK=0;           //���δ����ɱ�־λ
u8 waitservice_flag=0;   //�ȴ���������ʱ��־
u8 LEDshine_flag=0;     
//ͨ�ö�ʱ��3�жϳ�ʼ��
//����ʱ��ѡ��ΪAPB1��2������APB1Ϊ36M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��


void TIM3_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //ʱ��ʹ��

    //��ʱ��TIM3��ʼ��
    TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ
    TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ

    TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //ʹ��ָ����TIM3�ж�,��������ж�

    //�ж����ȼ�NVIC����
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3�ж�
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //��ռ���ȼ�0��
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  //�����ȼ�3��
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
    NVIC_Init(&NVIC_InitStructure);  //��ʼ��NVIC�Ĵ���

    TIM_Cmd(TIM3, ENABLE);  //ʹ��TIMx
}

//��ʱ��3�жϷ������
//ÿ100ms��һ���жϷ�����
void TIM3_IRQHandler(void)
{
    static u32 times=0;
    static u8 tt=0;
    static u32 waitservicetime = 0;  //�������ȴ��ļ�ʱ
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)    //�Ǹ����ж�
    {
        TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //���TIMx�����жϱ�־
        times++;

        if(Error==0)   //û�г���
        {
            IWDG_Feed(); //ι��
        }
        switch(deviceState)
        {
        case 0:
            break;
        case 1:
            break;
        case 2:
            break;
        case 4:
            break;
        case 5:    //��������״̬ gps��
        {
			if(times%10== 0)  //ÿһ����һ�ε�
			{
				LEDshine_flag=1;
			}
            if(submit_info_flag==0)
            {
                tt++;
                if(tt%300==0)   //ÿ30���ϴ�һ��
                {
                    submit_info_flag=1;
                }
            }

            if(times%100==0)       //ÿ��������1s
            {
                if(GPS_effective==1)    //GPS������Ч
                {
                    GPS_effective=0;    //GPS���ݱ��Ϊ��Ч
                    if(times%15==0)     //ÿ15s���һ���˲���Ϣ
                    {
                        //ֻ��ǰ�����εľ��볬��40m�ű�֤�������
                        getFilterLoc(&LatNow, &LonNow);   //�õ��˲���ľ�γ��  �������洢�����ĵ�ַ���룬����һ��ָ�����
                        printf("�˲���İٶȵ�ͼγ��%f,����%f\r\n",LatNow,LonNow);
                        distance=Cal_Distance(LatOld,LonOld,LatNow,LonNow)*1000;       //�������ζ�λ��   (��һ�ζ�λ�ľ�����ܴ�)
                        printf("\r\n�����ϴ��ľ���Ϊ%lfm\r\n",distance);
                        LatOld=LatNow;           //������һ�μ������
                        LonOld=LonNow;
//											if((40<distance)&&(distance<1000))     //ֻ�е���������ʱ,�Ż���
                        {
                            GPS_Upload=1;  			 //GPS�����־λ
                        }
                    }
                    //GPS��Ϣ������Чʱ �л���GPS
                    //							if()   //������ʲôʲô��λ��Ϣ  AGPS  GPS
                    //							{
                    //								//�ر�AGPS
                    //							}

                    GPS_invalidtimes=0;   //GPSδ��λ��ʱ����0
                }
//								else
//								{
//										GPS_invalidtimes++;
//										printf("δ�ɹ���λ��ʱ��%d\r\n\r\n",GPS_invalidtimes);
//										if(GPS_invalidtimes>=GPS_Switch_AGPS)   //����30s�յ�GPS����
//										{
//												//ֻ������GPSʱת��AGPSʱ�Ž���
//												printf("����AGPS�������綨λ\r\n\r\n");
//												AGPS_Location();

//										}
//										if(GPS_invalidtimes>=GPSFailedTime_MAX)  //����5����û�ж�λ�ɹ�
//										{
//												printf("GPS��λ��ʱ\r\n\r\n");
//												sysData_Pack.data.GPSErrorTimes++;
//												saveData();//������д��FLASH
//												GPS_invalidtimes=0;          //δ��λʱ����0   û����0�Ļ���������ݲ���
//		//                    Error=1;   //��־���Ź����ȴ���λ
//										}
//								}

//								Upload_Time++;   //�ϴ�ʱ����

//								if(Upload_Time>=Upload_Time_MAX)    //����5����û���ϴ�
//								{
//										Heartbeat_Upload=1;   //�����ϴ���־
//								}

//								if(Upload_Time>=Upload_Time_MAX+120)    //����6����û���ϴ�
//								{
//										//֤������û���ϴ��ɹ�
////										Error=1;   //��־���Ź����ȴ���λ
//								}

            }
            if(waitservice_flag==0)// ��־λ��������Ϊ�ȴ������м�ʱ�ȴ�
            {
                waitservicetime++;
                if((waitservicetime%100)==0)
                {
                    waitservice_flag=1;
                }
            }
            break;
        }
        }
    }
}





















//��ʱ��7�жϷ������
void TIM7_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)//�Ǹ����ж�
    {
        u2_data_Pack.USART2_RX_STA|=1<<15;	//��ǽ������
        TIM_ClearITPendingBit(TIM7, TIM_IT_Update  );  //���TIM7�����жϱ�־
        TIM_Cmd(TIM7, DISABLE);  //�ر�TIM7
    }
}

//ͨ�ö�ʱ��7�жϳ�ʼ��
//����ʱ��ѡ��ΪAPB1��2������APB1Ϊ42M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//��ʱ�����ʱ����㷽��:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=��ʱ������Ƶ��,��λ:Mhz
//ͨ�ö�ʱ���жϳ�ʼ��
//����ʼ��ѡ��ΪAPB1��2������APB1Ϊ36M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
void TIM7_Int_Init(u16 arr,u16 psc)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);//TIM7ʱ��ʹ��

    //��ʱ��TIM7��ʼ��
    TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ
    TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
    TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ

    TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE ); //ʹ��ָ����TIM7�ж�,��������ж�

    TIM_Cmd(TIM7,ENABLE);//������ʱ��7

    NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;//��ռ���ȼ�0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
    NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���

}







