#include "A7.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "delay.h"
#include "timer.h"
#include "lcd.h"
#include "usart.h"
#include "usart_2.h"
#include "stmflash.h"

u8 Post_Errotimes=0; //����A7����һ���ϴ�ʧ�ܴ���

u8 deviceState = 0;  //�豸��ǰ״̬ 0:���� 1:��ʼ��ATָ�� 2:���SIM�� 3:�����ź� 4:��ʼ��GPRS̬ 5:��������̬
Data_Pack u2_data_Pack;  //A7�����Ϣ


char ipaddr[]= {"115.28.30.250"};	//IP��ַ
char port[]= {"8077"};				//�˿ں�
char p1[300];
char POSTDATA[1000]= {0};


//��ʼ��A7����
void A7_Init()
{
    s8 ret;
    u32 i = 0;
    deviceState = 0;  //����̬
	Post_Errotimes=0;  
    saveData();//������д��FLASH
    Restart_A7();
    deviceState=1; //��ʼ��ATָ��̬
    /*��ʼ��ATָ�*/
    i = 0;
    while((ret = sendAT("AT\r\n","OK",500)) != 0)
    {
        printf("���ڳ�ʼ��ATָ�```%d\r\n",i++);
        if(i >= 50)
        {
            printf("\r\n��ʼ��ATָ�ʧ�ܣ�����������Ƭ��\r\n");
            u2_data_Pack.Error = 1;//��ǳ�ʼ��Ai_A6�����ȴ����Ź���λ��Ƭ��
            while(1);	//�ȴ����Ź���λ
        }
    }
    if(ret != 0) {
        printf("��ʼ��ATָ�ʧ��\r\n");
    }
    else
    {
        printf("��ʼ��ATָ��ɹ�\r\n");
        /*�رջ���*/
        ret=sendAT("ATE0","OK",5000);
        if(ret == 0) {
            printf("�رջ��Գɹ�\r\n");
        }
        else {
            printf("�رջ���ʧ��\r\n");
        }

        /*���ϱ���ϸ������Ϣ*/
        ret=sendAT("AT+CMEE=2","OK",5000);
        if(ret == 0) {
            printf("���ϱ���ϸ������Ϣ�ɹ�\r\n");
        }
        else {
            printf("���ϱ���ϸ������Ϣʧ��\r\n");
        }

        /*��ѯģ������汾*/
        checkRevision();

        /*��ѯIMEI��*/
        getIMEI();
        deviceState = 2;//���SIM��̬
        /*��ѯ�Ƿ��⵽SIM��*/
        i = 0;
        while((ret = sendAT("AT+CPIN?","READY",3000)) != 0)//ֻ�в���SIM���Ž��н������Ĳ���
        {
            printf("���ڲ�ѯSIM��״̬```%d\r\n",i++);
            if(i >= 20)
            {
                printf("\r\n���SIM��ʧ�ܣ�����������Ƭ��\r\n");
                u2_data_Pack.Error = 1;//��ǳ�ʼ��SIM808�����ȴ����Ź���λ��Ƭ��
                while(1);	//�ȴ����Ź���λ
            }
        }
        if(ret == 0) {
            printf("�Ѳ���SIM��\r\n");
        }
        else {
            printf("δ����SIM��\r\n");
        }

        /*��ѯ����ע�����*/
        ret = sendAT("AT+CREG?","CREG",3000);
        if(ret == 0)
        {
            printf("�����Ѿ�ע�ᣬ������ģʽ��");
        }
        else
        {
            ret = sendAT("AT+CREG=1","OK",3000);
            printf("�����Ѿ�ע�ᣬ������ģʽ��\r\n");
        }
        deviceState = 3;//�����ź�̬
        /*����ź�*/
        ret = sendAT("AT+COPS=0,0","OK",5000);
        if(ret == 0) {
            printf("���ó���ʽ��ĸ���ֳɹ�\r\n");
        }
        else {
            printf("���ó���ʽ��ĸ����ʧ��\r\n");
        }
        i = 0;
        while((ret = sendAT("AT+COPS?","OK",5000)) != 0)//ֻ���������źŲŽ��н������Ĳ���
        {
            printf("���ڼ���ź�```%d   %s\r\n",i++,my_strstr("+COPS:"));
            if(i >= 120)
            {
                printf("\r\n�����ź�ʧ�ܣ�����������Ƭ��\r\n");
                u2_data_Pack.Error = 1;//��ǳ�ʼ��SIM808�����ȴ����Ź���λ��Ƭ��
            }
            delay_ms(50);
        }
        if(ret == 0)
        {
            strncpy(u2_data_Pack.Operator, my_strstr("+COPS:") + 12, 11);
            u2_data_Pack.Operator[12] = '\0';//����ַ���������
            printf("����źųɹ� %s\r\n",u2_data_Pack.Operator);
        }
        deviceState = 4;//��ʼ��GPS̬
        A7_GPS_Init();
        deviceState = 5;//��������̬
    }
}
//��ѯģ���Ƿ���GPRS����
//�����Ƿ���GPRS���� 0������ -1��δ����
s8 A7_GPRS_Test(void)
{
    s8 ret;
    s32 signalQuality = 0;//�ź�����
    char * str = 0;
    /*��ѯ�ź�����*/
    ret = sendAT("AT+CSQ","OK",5000);
    if(ret != 0) {
        printf("��ѯ�ź�����ʧ��\r\n");
    }
    else
    {
        if(extractParameter(my_strstr("+CSQ:"),"+CSQ: ",&signalQuality,0,0) == 0)
        {
            printf("�ź�������%d\r\n",signalQuality);
        }
    }
    /*��ѯģ���Ƿ���GPRS����*/
    ret = sendAT("AT+CGATT?","+CGATT:",1000);
    if(ret != 0) {
        return -1;
    }
    str = my_strstr("+CGATT:");
    if(str[7] == '1')//�Ѹ���GPSR����
    {
        //printf("����GPRS����ɹ�\r\n");
        return 0;
    }
    else//δ����GPSR���磬���Ը���GPRS����
    {
        return GPRS_Connect();
    }
}

//�ڴ��ڽ��յ��ַ���������
//����ֵ		�ɹ����ر������ַ������׵�ַ
//			ʧ�ܷ��� 0
char * my_strstr(char *searchStr)
{

    char * ret = 0;

    if((u2_data_Pack.USART2_RX_STA&(1<<15))!=0)	//�������
    {
        if((u2_data_Pack.USART2_RX_STA|(0x8FFF)) >= USART2_MAX_RECV_LEN)
        {
            u2_data_Pack.USART2_RX_BUF[USART2_MAX_RECV_LEN-1] = '\0';
        }
        ret = strstr((char *)u2_data_Pack.USART2_RX_BUF,searchStr);
        if(ret != 0)
        {
            return ret;
        }
    }

    return 0;
}

//���������
//��������
//����ֵ ��
void cleanReceiveData(void)
{
    u16 i;
    u2_data_Pack.USART2_RX_STA=0;			//���ռ���������
    for(i = 0; i < USART2_MAX_RECV_LEN; i++)
    {
        u2_data_Pack.USART2_RX_BUF[i] = 0;
    }
}



//����һ��ATָ�������Ƿ��յ�ָ����Ӧ��
//sendStr:���͵������ַ���,��sendStr<0XFF��ʱ��,��������(���緢��0X1A),���ڵ�ʱ�����ַ���.
//searchStr:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//outTime:�ȴ�ʱ��(��λ:1ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       -1,����ʧ��
s8 sendAT(char *sendStr,char *searchStr,u32 outTime)
{
    s8 ret = 0;
    char * res ;
    delay_ms(300);
    cleanReceiveData();//���������
    if((u32)sendStr < 0xFF)
    {
        while((USART2->SR&0X40)==0);//�ȴ���һ�����ݷ������
        USART2->DR=(u32)sendStr;
    }
    else
    {
        u2_printf(sendStr);//����ATָ��
        u2_printf("\r\n");//���ͻس�����
    }
    if(searchStr && outTime)//��searchStr��outTime��Ϊ0ʱ�ŵȴ�Ӧ��
    {
        while((--outTime)&&(res == 0))//�ȴ�ָ����Ӧ���ʱʱ�䵽��
        {
            res = my_strstr(searchStr);
            delay_ms(1);
        }
        if(outTime == 0) {
            ret = -1;    //��ʱ
        }
        if(res != 0)//res��Ϊ0֤���յ�ָ��Ӧ��
        {
            ret = 0;
        }
    }
    return ret;
}


//��ѯģ��汾��
void checkRevision(void)
{
    s8 ret;
    char * str;
    /*��ѯģ������汾*/
    ret = sendAT("AT+GMR","OK",5000);
    if(ret == 0) {
        printf("��ѯģ������汾�ɹ�\r\n");
    }
    else
    {
        printf("��ѯģ������汾ʧ��\r\n");
        u2_data_Pack.Error = 1;//��ǳ�ʼ��Ai_A6�����ȴ����Ź���λ��Ƭ��
    }
    str = my_strstr("V");
    if(str != 0)
    {
        strncpy(u2_data_Pack.revision, str , 20);
        u2_data_Pack.revision[20] = '\0';//����ַ���������
        printf("ģ������汾: %s  \r\n",u2_data_Pack.revision);
    }
    else
    {
        printf("��ѯģ������汾ʧ��\r\n");
        u2_data_Pack.Error = 1;//��ǳ�ʼ��Ai_A6�����ȴ����Ź���λ��Ƭ��
    }
}

//��ѯIMEI
void getIMEI(void)
{
    s8 ret;
    char * str;
    /*��ѯIMEI��*/
    ret = sendAT("AT+GSN","OK",5000);
    if(ret == 0) {  }
    else
    {
        printf("��ѯIMEI��ʧ��\r\n");
        u2_data_Pack.Error = 1;//��ǳ�ʼ��Ai_A6�����ȴ����Ź���λ��Ƭ��
    }
    str = my_strstr("OK");
    if(str != 0)
    {
        strncpy(u2_data_Pack.IMEI, str - 19, 15);
        u2_data_Pack.IMEI[15] = '\0';//����ַ���������
        printf("IMEI��: %s\r\n",u2_data_Pack.IMEI);
    }
}



//GPRS����
s8 GPRS_Connect(void)
{
    s8 ret;
    ret = sendAT("AT+CIPCLOSE=1","CLOSE OK",1000);	//�ر�����
    if(ret == 0) {
        printf("�ر����ӳɹ�\r\n");
    }
    else {
        printf("�ر�����ʧ��\r\n");
    }

    ret = sendAT("AT+CIPSHUT","SHUT OK",1000);		//�ر��ƶ�����
    if(ret == 0) {
        printf("�ر��ƶ������ɹ�\r\n");
    }
    else {
        printf("�ر��ƶ�����ʧ��\r\n");
    }

    ret = sendAT("AT+CGCLASS=\"B\"","OK",1000);		//����GPRS�ƶ�̨���ΪB,֧�ְ����������ݽ���
    if(ret == 0) {
        printf("����GPRS�ƶ�̨����ΪB�ɹ�\r\n");
    }
    else {
        printf("����GPRS�ƶ�̨����ΪBʧ��\r\n");
    }

    ret = sendAT("AT+CGATT=1","OK",5000);			//����GPRSҵ��
    if(ret == 0) {
        printf("����GPRSҵ��ɹ�\r\n");
    }
    else {
        printf("����GPRSҵ��ʧ��\r\n");
    }

    ret = sendAT("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",5000);	//����PDP����
    if(ret == 0) {
        printf("����PDP�����ɹ�\r\n");
    }
    else {
        printf("����PDP����ʧ��\r\n");
    }

    ret = sendAT("AT+CGACT=1,1 ","OK",5000);			//����PDP����ȷ�����Ժ�Ϳ���������
    if(ret == 0) {
        printf("����PDP�ɹ�\r\n");
    }
    else {
        printf("����PDPʧ��\r\n");
    }
    return ret;
}
s8 TCP_Connect(void)
{
    s8 ret;
    char p[100];
    sprintf((char*)p,"AT+CIPSTART=\"TCP\",\"%s\",%s",ipaddr,port);
    ret = sendAT(p,"OK",5000);	//����TCP����
    if(ret == 0)
    {
        printf("����TCP�����ӳɹ�\r\n");
    }
    else
    {
        printf("����TCP������ʧ��\r\n");
    }
    return ret;
}
//��鵱ǰ����״̬�����ݵ�ǰ״̬���в���
//CONNECK OK:TCP������
//IP CLOSE :TCP���ӱ��ر�
//GPRSACT:GPRS�����ӣ�TCPδ����
//INITIAL:GPRSδ����
//����״̬��������һ��
s8 Check_TCP(void)
{
    s8 ret;
    u8 state=0;
    if((ret=sendAT("AT+CIPSTATUS","CONNECT OK",2000))==0)
    {
        printf("��ǰ����״̬:CONNECK OK\r\n");
        state=5;
    }
    else if(my_strstr("CLOSE")!=0)
    {
        printf("��ǰ����״̬:IP CLOSE\r\n");
        state=4;
    }
    else if(my_strstr("GPRSACT")!=0)
    {
        printf("��ǰ����״̬:GPRSACT\r\n");
        state=3;
    }
    else if(my_strstr("INITIAL")!=0)
    {
        printf("��ǰ����״̬:INITIAL\r\n");
        state=2;
    }
    else
    {
        printf("��ǰ����״̬:δ֪\r\n");
        state=1;
    }
    switch(state)
    {
    case 1:
    case 2:
        GPRS_Connect();
    case 3:
    case 4:
        if((ret=TCP_Connect())==0) {
            printf("TCP���ӳɹ�\r\n");
        }
        else {
            printf("TCP����ʧ��\r\n");
        }
    default:
        break;
    }
    return ret;
}


//POST /WebService.asmx/Get_RandomNumber HTTP/1.1
//Host: 115.28.30.250
//Content-Type: application/x-www-form-urlencoded
//Content-Length: length

//Hand=string&Name=string

u8 isSendData = 0;		//�Ƿ��ںͷ�����ͨ�ŵı�־λ
u8 isSendDataError = 0;	//�ϴ��������Ƿ�ʧ�� 0:Ϊ�ɹ� 1:ʧ��

char SubWebService[]= {"WebService.asmx/Submit_ces"};
char keystr[20]= {"2-409"};
//POST��ʽ�ύһ������
//����ֵ 0 �ɹ� ��-1ʧ��


s8 HTTP_POST(char * GPS_Info)
{
    u8 i=0;
    s8 ret;

    u16 length;
    isSendData = 1;		//�ںͷ�����ͨ�ŵı�־λ

    while((ret=Check_TCP())!=0)
    {
        i++;
        printf("TCP���ӳ����쳣...%c\r\n",i);
        if(i>10)
        {
            printf("TCP���ӹ���...׼������\r\n");

        }
        delay_ms(50);
    }

    sprintf(p1,"log=%s&key=%s",GPS_Info,keystr);
    printf("%s\r\n",p1);
    length=strlen(p1);
    printf("%d\r\n",length);
    sprintf(POSTDATA,"POST /%s HTTP/1.1\r\nHost: %s\r\n\
Content-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n",
            SubWebService,ipaddr,length,p1	);

    if(sendAT("AT+CIPSEND",">",5000)==0)		//��������
    {
        printf("CIPSEND DATA:%s\r\n",p1);	 			//�������ݴ�ӡ������

        u2_printf("%s\r\n",POSTDATA);
        delay_ms(10);
        if(sendAT((char*)0X1A,"OK",5000)==0)
        {
            cleanReceiveData();//���������
            printf("POST�ɹ�\r\n");
            ret= 0;
        }
        else
        {
            isSendDataError = 1;//���ñ�־λΪʧ��
            printf("POSTʧ��\r\n");
            ret= -1;
        }

    } else
    {
        sendAT((char*)0X1B,0,0);	//ESC,ȡ������
        isSendDataError = 1;//���ñ�־λΪʧ��
        printf("POSTʧ�ܣ�ȡ������");
        ret= -1;
    }

    isSendData = 0;//ת����־λΪδ���������ͨ��
    return ret;
}


//HTTP/1.1 200 OK
//Content-Type: text/xml; charset=utf-8
//Content-Length: length

//<?xml version="1.0" encoding="utf-8"?>
//<string xmlns="http://tempuri.org/">string</string>
//�������ز���
char * AnalyticalData(void)
{
    u16 i;
    char * retHead = 0;
    char * retTail = 0;
    if(my_strstr("HTTP/1.1 200 OK")!=0)
    {
        retTail=my_strstr("</string>");
        if(retTail!=0)
        {
            for(i=0; i<USART2_MAX_RECV_LEN; i++)
            {
                if( *(retTail-i)=='>' )
                {
                    retHead=retTail-i+1;
                    * retTail=0;
					delay_ms(20);
                    LCD_SString(10,100,300,200,12,u2_data_Pack.USART2_RX_BUF);		//��ʾһ���ַ���,12/16����
                    return retHead;
                }
            }
        }


    }
    return 0;
}

//char *Analytica1Data_GET(void)
//{
//    u16 i;
//    char * retHead = 0;
//    char * retTail = 0;
//    if((retTail=my_strstr("+CIPRCV:"))!=0)
//    {
//        for(i=0; *(retTail+i)!=NULL; i++)
//        {
//            if( *(retTail+i)==',' )
//            {
//                retHead=retTail+i+1;
//                * retTail=0;
//                return retHead;
//            }
//        }
//    }
//    return 0;
//}


//��ȡ�ַ����еĲ���(��������������������)
//str:Դ�ַ�����searchStr:����ǰ�������ַ���,data1:��ȡ���Ĳ���1 data2:��ȡ���Ĳ���2 data3:��ȡ���Ĳ���3
//�ɹ�����0 ʧ�ܷ���-1

s8 extractParameter(char *sourceStr,char *searchStr,s32 *dataOne,s32 *dataTwo,s32 *dataThree)
{
    char *str;
    char *commaOneAddr;//��һ�����ŵĵ�ַ
    char *commaTwoAddr;//�ڶ������ŵĵ�ַ
    char dataStrOne[10], dataStrTwo[10], dataStrThree[10];
    u32 commaNum = 0,lenthOne = 0,lenthTwo = 0,lenthThree = 0,lenthEigen = 0;

    str = strstr(sourceStr,searchStr);
    if(str == 0) {
        return -1;
    }
    else
    {
        commaNum = charNum(sourceStr,',');//�����ж��ٶ���
        if(commaNum == 1)//����������
        {
            lenthEigen = strlen(searchStr);//�õ������ַ����ĳ���
            commaOneAddr = ReturnCommaPosition(str,1);//�õ���һ������λ��
            lenthOne = (u32)(commaOneAddr - str) - lenthEigen;//�õ���һ�������ĳ���
            strncpy(dataStrOne, sourceStr + lenthEigen, lenthOne);
            dataStrOne[lenthOne] = '\0';//����ַ���������
            if(dataOne != 0) {
                *dataOne = atoi(dataStrOne);    //���ַ���ת��Ϊ����
            }

            lenthTwo = strlen(commaOneAddr + 1);//�õ��ڶ��������ĳ���
            strncpy(dataStrTwo, commaOneAddr + 1, lenthTwo);
            dataStrTwo[lenthTwo] = '\0';//����ַ���������
            if(dataTwo != 0) {
                *dataTwo = atoi(dataStrTwo);    //���ַ���ת��Ϊ����
            }
            return 0;
        }
        if(commaNum == 2)//����������
        {
            lenthEigen = strlen(searchStr);//�õ������ַ����ĳ���
            commaOneAddr = ReturnCommaPosition(str,1);//�õ���һ������λ��
            lenthOne = (u32)(commaOneAddr - str) - lenthEigen;//�õ���һ�������ĳ���
            strncpy(dataStrOne, sourceStr + lenthEigen, lenthOne);
            dataStrOne[lenthOne] = '\0';//����ַ���������
            if(dataOne != 0) {
                *dataOne = atoi(dataStrOne);    //���ַ���ת��Ϊ����
            }

            commaTwoAddr = ReturnCommaPosition(str,2);//�õ��ڶ�������λ��
            lenthTwo = (u32)(commaTwoAddr - commaOneAddr - 1);//�õ��ڶ��������ĳ���
            strncpy(dataStrTwo, commaOneAddr + 1, lenthTwo);
            dataStrOne[lenthTwo] = '\0';//����ַ���������
            if(dataTwo != 0) {
                *dataTwo = atoi(dataStrTwo);    //���ַ���ת��Ϊ����
            }

            lenthThree = strlen(commaTwoAddr + 1);//�õ��ڶ��������ĳ���
            strncpy(dataStrThree, commaTwoAddr + 1, lenthThree);
            dataStrThree[lenthTwo] = '\0';//����ַ���������
            if(dataThree != 0) {
                *dataThree = atoi(dataStrThree);    //���ַ���ת��Ϊ����
            }
            return 0;
        }
    }
    return -1;
}


//�������ƣ����ض���λ��
//���������p	������ַ����׵�ַ
//		  num	�ڼ�������
//���ز�������num�����ŵ�ַ
//˵    ������65535���ַ���û�ж��žͷ����ַ����׵�ַ
char * ReturnCommaPosition(char *p ,u8 num)
{
    u8 i=0;			//��i������
    u16 k=0;		//��ַ����ֵ
    while(k<65535)
    {
        if( *(p+k) ==',')
        {
            i++;
            if(i==num)
            {
                return p+k;
            }
        }
        k++;
    }
    return p;
}
//�������ƣ������ַ������ж��ٸ�ָ���ַ�
//���������p	������ַ����׵�ַ
//		  	Char	���������ַ�
//���ز������ַ������ж��ٸ�ָ���ַ�

u8 charNum(char *p ,char Char)
{
    u8 i=0;			//��i������
    u16 k=0;		//��ַ����ֵ
    while(1)
    {
        if( *(p+k) == Char)
        {
            i++;
        }
        else if(*(p+k) == '\0')
        {
            return i;
        }
        k++;
    }
}

//Ӳ������A7
void Restart_A7(void)
{
}

//��ʼ��A7GPS
void A7_GPS_Init(void)
{
	u8 i=0;
    while(sendAT("AT+GPS=1","OK",5000)!=0)
	{
       i++;
		printf("GPS����ʧ�ܴ���...%c\r\n",i);
		if(i>20)
			A7_Init();//����A7
	}
	printf("GPS�򿪳ɹ�\r\n");
}


//�˺������ڷ��Ͳ���У�����������������Ϣ
//��������Ϊ���ȷ����������ݣ�����ɹ��������ŷ���flash����δ�ɹ������ݣ�
//���ʧ�ܣ��ͽ�failedTimes++,failedTimes���Ϊ30��
void A7_SendPost()  //���Ͳ�У��
{
    s8 ret,yl;
	LCD_Clear(WHITE);   //����
    if((ret = HTTP_POST(TEXT_Buffer))==0)
        printf("���ͳɹ�\r\n");
    else
        printf("����ʧ��\r\n");

    if(!(isSendDataError || isSendData) )
    {
        waitservice_flag=0;
        while(!(waitservice_flag ||(u2_data_Pack.USART2_RX_STA&(1<<15))));//�ȴ���������Ӧ,�������
        
        
        if(AnalyticalData()!=0)
        {
            isSendDataError = 0;//���ñ�־λΪ�ɹ�
            printf("�յ���ȷӦ��\r\n");
            if(sysData_Pack.data.failedTimes!=0)     //FLASH�д���δ�ɹ��ϴ�����
            {
                for(yl=1; yl<=sysData_Pack.data.failedTimes; yl++) //��failedTimes�ϴ�������
                {
                    FLASH_GPS_Read(yl);     //��ȡFLASHδ�ϴ��ɹ�����
                    if((ret = HTTP_POST(datatemp))==0)
                        printf("���ͳɹ�\r\n");
                    else
                        printf("����ʧ��\r\n");
                }
				         sysData_Pack.data.failedTimes=0;
            }
        }
        else
        {
            isSendDataError = 1;
            printf("δ�յ���ȷӦ��\r\n");
        }
    }
    ret = sendAT("AT+CIPCLOSE=1","OK",2000);	//�ر�����
    if(ret == 0)
    {
        printf("�ر����ӳɹ�\r\n");
    }
    else
    {
        printf("�ر�����ʧ��\r\n");
    }

}


