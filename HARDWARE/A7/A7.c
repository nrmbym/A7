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

u8 Post_Errotimes=0; //本次A7开机一共上传失败次数

u8 deviceState = 0;  //设备当前状态 0:开机 1:初始化AT指令 2:检查SIM卡 3:搜索信号 4:初始化GPRS态 5:正常工作态
Data_Pack u2_data_Pack;  //A7相关信息


char ipaddr[]= {"115.28.30.250"};	//IP地址
char port[]= {"8077"};				//端口号
char p1[300];
char POSTDATA[1000]= {0};


//初始化A7函数
void A7_Init()
{
    s8 ret;
    u32 i = 0;
    deviceState = 0;  //开机态
	Post_Errotimes=0;  
    saveData();//将数据写入FLASH
    Restart_A7();
    deviceState=1; //初始化AT指令态
    /*初始化AT指令集*/
    i = 0;
    while((ret = sendAT("AT\r\n","OK",500)) != 0)
    {
        printf("正在初始化AT指令集```%d\r\n",i++);
        if(i >= 50)
        {
            printf("\r\n初始化AT指令集失败，正在重启单片机\r\n");
            u2_data_Pack.Error = 1;//标记初始化Ai_A6出错，等待看门狗复位单片机
            while(1);	//等待看门狗复位
        }
    }
    if(ret != 0) {
        printf("初始化AT指令集失败\r\n");
    }
    else
    {
        printf("初始化AT指令集成功\r\n");
        /*关闭回显*/
        ret=sendAT("ATE0","OK",5000);
        if(ret == 0) {
            printf("关闭回显成功\r\n");
        }
        else {
            printf("关闭回显失败\r\n");
        }

        /*打开上报详细错误信息*/
        ret=sendAT("AT+CMEE=2","OK",5000);
        if(ret == 0) {
            printf("打开上报详细错误信息成功\r\n");
        }
        else {
            printf("打开上报详细错误信息失败\r\n");
        }

        /*查询模块软件版本*/
        checkRevision();

        /*查询IMEI号*/
        getIMEI();
        deviceState = 2;//检测SIM卡态
        /*查询是否检测到SIM卡*/
        i = 0;
        while((ret = sendAT("AT+CPIN?","READY",3000)) != 0)//只有插入SIM卡才进行接下来的操作
        {
            printf("正在查询SIM卡状态```%d\r\n",i++);
            if(i >= 20)
            {
                printf("\r\n检测SIM卡失败，正在重启单片机\r\n");
                u2_data_Pack.Error = 1;//标记初始化SIM808出错，等待看门狗复位单片机
                while(1);	//等待看门狗复位
            }
        }
        if(ret == 0) {
            printf("已插入SIM卡\r\n");
        }
        else {
            printf("未插入SIM卡\r\n");
        }

        /*查询网络注册情况*/
        ret = sendAT("AT+CREG?","CREG",3000);
        if(ret == 0)
        {
            printf("网络已经注册，在漫游模式下");
        }
        else
        {
            ret = sendAT("AT+CREG=1","OK",3000);
            printf("网络已经注册，在漫游模式下\r\n");
        }
        deviceState = 3;//搜索信号态
        /*检测信号*/
        ret = sendAT("AT+COPS=0,0","OK",5000);
        if(ret == 0) {
            printf("设置长格式字母数字成功\r\n");
        }
        else {
            printf("设置长格式字母数字失败\r\n");
        }
        i = 0;
        while((ret = sendAT("AT+COPS?","OK",5000)) != 0)//只有搜索到信号才进行接下来的操作
        {
            printf("正在检测信号```%d   %s\r\n",i++,my_strstr("+COPS:"));
            if(i >= 120)
            {
                printf("\r\n搜索信号失败，正在重启单片机\r\n");
                u2_data_Pack.Error = 1;//标记初始化SIM808出错，等待看门狗复位单片机
            }
            delay_ms(50);
        }
        if(ret == 0)
        {
            strncpy(u2_data_Pack.Operator, my_strstr("+COPS:") + 12, 11);
            u2_data_Pack.Operator[12] = '\0';//添加字符串结束符
            printf("检测信号成功 %s\r\n",u2_data_Pack.Operator);
        }
        deviceState = 4;//初始化GPS态
        A7_GPS_Init();
        deviceState = 5;//正常工作态
    }
}
//查询模块是否附着GPRS网络
//返回是否附着GPRS网络 0：附着 -1：未附着
s8 A7_GPRS_Test(void)
{
    s8 ret;
    s32 signalQuality = 0;//信号质量
    char * str = 0;
    /*查询信号质量*/
    ret = sendAT("AT+CSQ","OK",5000);
    if(ret != 0) {
        printf("查询信号质量失败\r\n");
    }
    else
    {
        if(extractParameter(my_strstr("+CSQ:"),"+CSQ: ",&signalQuality,0,0) == 0)
        {
            printf("信号质量：%d\r\n",signalQuality);
        }
    }
    /*查询模块是否附着GPRS网络*/
    ret = sendAT("AT+CGATT?","+CGATT:",1000);
    if(ret != 0) {
        return -1;
    }
    str = my_strstr("+CGATT:");
    if(str[7] == '1')//已附着GPSR网络
    {
        //printf("附着GPRS网络成功\r\n");
        return 0;
    }
    else//未附着GPSR网络，尝试附着GPRS网络
    {
        return GPRS_Connect();
    }
}

//在串口接收的字符串中搜索
//返回值		成功返回被查找字符串的首地址
//			失败返回 0
char * my_strstr(char *searchStr)
{

    char * ret = 0;

    if((u2_data_Pack.USART2_RX_STA&(1<<15))!=0)	//接收完成
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

//清除接收器
//参数：无
//返回值 无
void cleanReceiveData(void)
{
    u16 i;
    u2_data_Pack.USART2_RX_STA=0;			//接收计数器清零
    for(i = 0; i < USART2_MAX_RECV_LEN; i++)
    {
        u2_data_Pack.USART2_RX_BUF[i] = 0;
    }
}



//发送一条AT指令，并检测是否收到指定的应答
//sendStr:发送的命令字符串,当sendStr<0XFF的时候,发送数字(比如发送0X1A),大于的时候发送字符串.
//searchStr:期待的应答结果,如果为空,则表示不需要等待应答
//outTime:等待时间(单位:1ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       -1,发送失败
s8 sendAT(char *sendStr,char *searchStr,u32 outTime)
{
    s8 ret = 0;
    char * res ;
    delay_ms(300);
    cleanReceiveData();//清除接收器
    if((u32)sendStr < 0xFF)
    {
        while((USART2->SR&0X40)==0);//等待上一次数据发送完成
        USART2->DR=(u32)sendStr;
    }
    else
    {
        u2_printf(sendStr);//发送AT指令
        u2_printf("\r\n");//发送回车换行
    }
    if(searchStr && outTime)//当searchStr和outTime不为0时才等待应答
    {
        while((--outTime)&&(res == 0))//等待指定的应答或超时时间到来
        {
            res = my_strstr(searchStr);
            delay_ms(1);
        }
        if(outTime == 0) {
            ret = -1;    //超时
        }
        if(res != 0)//res不为0证明收到指定应答
        {
            ret = 0;
        }
    }
    return ret;
}


//查询模块版本号
void checkRevision(void)
{
    s8 ret;
    char * str;
    /*查询模块软件版本*/
    ret = sendAT("AT+GMR","OK",5000);
    if(ret == 0) {
        printf("查询模块软件版本成功\r\n");
    }
    else
    {
        printf("查询模块软件版本失败\r\n");
        u2_data_Pack.Error = 1;//标记初始化Ai_A6出错，等待看门狗复位单片机
    }
    str = my_strstr("V");
    if(str != 0)
    {
        strncpy(u2_data_Pack.revision, str , 20);
        u2_data_Pack.revision[20] = '\0';//添加字符串结束符
        printf("模块软件版本: %s  \r\n",u2_data_Pack.revision);
    }
    else
    {
        printf("查询模块软件版本失败\r\n");
        u2_data_Pack.Error = 1;//标记初始化Ai_A6出错，等待看门狗复位单片机
    }
}

//查询IMEI
void getIMEI(void)
{
    s8 ret;
    char * str;
    /*查询IMEI号*/
    ret = sendAT("AT+GSN","OK",5000);
    if(ret == 0) {  }
    else
    {
        printf("查询IMEI号失败\r\n");
        u2_data_Pack.Error = 1;//标记初始化Ai_A6出错，等待看门狗复位单片机
    }
    str = my_strstr("OK");
    if(str != 0)
    {
        strncpy(u2_data_Pack.IMEI, str - 19, 15);
        u2_data_Pack.IMEI[15] = '\0';//添加字符串结束符
        printf("IMEI号: %s\r\n",u2_data_Pack.IMEI);
    }
}



//GPRS连接
s8 GPRS_Connect(void)
{
    s8 ret;
    ret = sendAT("AT+CIPCLOSE=1","CLOSE OK",1000);	//关闭连接
    if(ret == 0) {
        printf("关闭连接成功\r\n");
    }
    else {
        printf("关闭连接失败\r\n");
    }

    ret = sendAT("AT+CIPSHUT","SHUT OK",1000);		//关闭移动场景
    if(ret == 0) {
        printf("关闭移动场景成功\r\n");
    }
    else {
        printf("关闭移动场景失败\r\n");
    }

    ret = sendAT("AT+CGCLASS=\"B\"","OK",1000);		//设置GPRS移动台类别为B,支持包交换和数据交换
    if(ret == 0) {
        printf("设置GPRS移动台类型为B成功\r\n");
    }
    else {
        printf("设置GPRS移动台类型为B失败\r\n");
    }

    ret = sendAT("AT+CGATT=1","OK",5000);			//附着GPRS业务
    if(ret == 0) {
        printf("附着GPRS业务成功\r\n");
    }
    else {
        printf("附着GPRS业务失败\r\n");
    }

    ret = sendAT("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",5000);	//设置PDP参数
    if(ret == 0) {
        printf("设置PDP参数成功\r\n");
    }
    else {
        printf("设置PDP参数失败\r\n");
    }

    ret = sendAT("AT+CGACT=1,1 ","OK",5000);			//激活PDP，正确激活以后就可以上网了
    if(ret == 0) {
        printf("激活PDP成功\r\n");
    }
    else {
        printf("激活PDP失败\r\n");
    }
    return ret;
}
s8 TCP_Connect(void)
{
    s8 ret;
    char p[100];
    sprintf((char*)p,"AT+CIPSTART=\"TCP\",\"%s\",%s",ipaddr,port);
    ret = sendAT(p,"OK",5000);	//建立TCP连接
    if(ret == 0)
    {
        printf("建立TCP的连接成功\r\n");
    }
    else
    {
        printf("建立TCP的连接失败\r\n");
    }
    return ret;
}
//检查当前连接状态，根据当前状态进行操作
//CONNECK OK:TCP已连接
//IP CLOSE :TCP连接被关闭
//GPRSACT:GPRS已连接，TCP未连接
//INITIAL:GPRS未连接
//其他状态就再配置一次
s8 Check_TCP(void)
{
    s8 ret;
    u8 state=0;
    if((ret=sendAT("AT+CIPSTATUS","CONNECT OK",2000))==0)
    {
        printf("当前连接状态:CONNECK OK\r\n");
        state=5;
    }
    else if(my_strstr("CLOSE")!=0)
    {
        printf("当前连接状态:IP CLOSE\r\n");
        state=4;
    }
    else if(my_strstr("GPRSACT")!=0)
    {
        printf("当前连接状态:GPRSACT\r\n");
        state=3;
    }
    else if(my_strstr("INITIAL")!=0)
    {
        printf("当前连接状态:INITIAL\r\n");
        state=2;
    }
    else
    {
        printf("当前连接状态:未知\r\n");
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
            printf("TCP连接成功\r\n");
        }
        else {
            printf("TCP连接失败\r\n");
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

u8 isSendData = 0;		//是否在和服务器通信的标志位
u8 isSendDataError = 0;	//上传服务器是否失败 0:为成功 1:失败

char SubWebService[]= {"WebService.asmx/Submit_ces"};
char keystr[20]= {"2-409"};
//POST方式提交一次数据
//返回值 0 成功 ；-1失败


s8 HTTP_POST(char * GPS_Info)
{
    u8 i=0;
    s8 ret;

    u16 length;
    isSendData = 1;		//在和服务器通信的标志位

    while((ret=Check_TCP())!=0)
    {
        i++;
        printf("TCP连接出现异常...%c\r\n",i);
        if(i>10)
        {
            printf("TCP连接故障...准备重启\r\n");

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

    if(sendAT("AT+CIPSEND",">",5000)==0)		//发送数据
    {
        printf("CIPSEND DATA:%s\r\n",p1);	 			//发送数据打印到串口

        u2_printf("%s\r\n",POSTDATA);
        delay_ms(10);
        if(sendAT((char*)0X1A,"OK",5000)==0)
        {
            cleanReceiveData();//清除接收器
            printf("POST成功\r\n");
            ret= 0;
        }
        else
        {
            isSendDataError = 1;//设置标志位为失败
            printf("POST失败\r\n");
            ret= -1;
        }

    } else
    {
        sendAT((char*)0X1B,0,0);	//ESC,取消发送
        isSendDataError = 1;//设置标志位为失败
        printf("POST失败，取消发送");
        ret= -1;
    }

    isSendData = 0;//转换标志位为未在与服务器通信
    return ret;
}


//HTTP/1.1 200 OK
//Content-Type: text/xml; charset=utf-8
//Content-Length: length

//<?xml version="1.0" encoding="utf-8"?>
//<string xmlns="http://tempuri.org/">string</string>
//解析返回参数
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
                    LCD_SString(10,100,300,200,12,u2_data_Pack.USART2_RX_BUF);		//显示一个字符串,12/16字体
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


//提取字符串中的参数(两个参数或者三个参数)
//str:源字符串，searchStr:参数前的特征字符串,data1:提取出的参数1 data2:提取出的参数2 data3:提取出的参数3
//成功返回0 失败返回-1

s8 extractParameter(char *sourceStr,char *searchStr,s32 *dataOne,s32 *dataTwo,s32 *dataThree)
{
    char *str;
    char *commaOneAddr;//第一个逗号的地址
    char *commaTwoAddr;//第二个逗号的地址
    char dataStrOne[10], dataStrTwo[10], dataStrThree[10];
    u32 commaNum = 0,lenthOne = 0,lenthTwo = 0,lenthThree = 0,lenthEigen = 0;

    str = strstr(sourceStr,searchStr);
    if(str == 0) {
        return -1;
    }
    else
    {
        commaNum = charNum(sourceStr,',');//搜索有多少逗号
        if(commaNum == 1)//有两个参数
        {
            lenthEigen = strlen(searchStr);//得到特征字符串的长度
            commaOneAddr = ReturnCommaPosition(str,1);//得到第一个逗号位置
            lenthOne = (u32)(commaOneAddr - str) - lenthEigen;//得到第一个参数的长度
            strncpy(dataStrOne, sourceStr + lenthEigen, lenthOne);
            dataStrOne[lenthOne] = '\0';//添加字符串结束符
            if(dataOne != 0) {
                *dataOne = atoi(dataStrOne);    //将字符串转换为整数
            }

            lenthTwo = strlen(commaOneAddr + 1);//得到第二个参数的长度
            strncpy(dataStrTwo, commaOneAddr + 1, lenthTwo);
            dataStrTwo[lenthTwo] = '\0';//添加字符串结束符
            if(dataTwo != 0) {
                *dataTwo = atoi(dataStrTwo);    //将字符串转换为整数
            }
            return 0;
        }
        if(commaNum == 2)//有三个参数
        {
            lenthEigen = strlen(searchStr);//得到特征字符串的长度
            commaOneAddr = ReturnCommaPosition(str,1);//得到第一个逗号位置
            lenthOne = (u32)(commaOneAddr - str) - lenthEigen;//得到第一个参数的长度
            strncpy(dataStrOne, sourceStr + lenthEigen, lenthOne);
            dataStrOne[lenthOne] = '\0';//添加字符串结束符
            if(dataOne != 0) {
                *dataOne = atoi(dataStrOne);    //将字符串转换为整数
            }

            commaTwoAddr = ReturnCommaPosition(str,2);//得到第二个逗号位置
            lenthTwo = (u32)(commaTwoAddr - commaOneAddr - 1);//得到第二个参数的长度
            strncpy(dataStrTwo, commaOneAddr + 1, lenthTwo);
            dataStrOne[lenthTwo] = '\0';//添加字符串结束符
            if(dataTwo != 0) {
                *dataTwo = atoi(dataStrTwo);    //将字符串转换为整数
            }

            lenthThree = strlen(commaTwoAddr + 1);//得到第二个参数的长度
            strncpy(dataStrThree, commaTwoAddr + 1, lenthThree);
            dataStrThree[lenthTwo] = '\0';//添加字符串结束符
            if(dataThree != 0) {
                *dataThree = atoi(dataStrThree);    //将字符串转换为整数
            }
            return 0;
        }
    }
    return -1;
}


//函数名称：返回逗号位置
//输入参数：p	待检测字符串首地址
//		  num	第几个逗号
//返回参数：第num个逗号地址
//说    明：若65535个字符内没有逗号就返回字符串首地址
char * ReturnCommaPosition(char *p ,u8 num)
{
    u8 i=0;			//第i个逗号
    u16 k=0;		//地址增加值
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
//函数名称：搜索字符串中有多少个指定字符
//输入参数：p	待检测字符串首地址
//		  	Char	待搜索的字符
//返回参数：字符串中有多少个指定字符

u8 charNum(char *p ,char Char)
{
    u8 i=0;			//第i个逗号
    u16 k=0;		//地址增加值
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

//硬件重启A7
void Restart_A7(void)
{
}

//初始化A7GPS
void A7_GPS_Init(void)
{
	u8 i=0;
    while(sendAT("AT+GPS=1","OK",5000)!=0)
	{
       i++;
		printf("GPS开启失败次数...%c\r\n",i);
		if(i>20)
			A7_Init();//重启A7
	}
	printf("GPS打开成功\r\n");
}


//此函数用于发送并且校验服务器返回来的信息
//发送流程为，先发送最新数据，如果成功，紧接着发送flash里面未成功的数据，
//如果失败，就将failedTimes++,failedTimes最大为30次
void A7_SendPost()  //发送并校验
{
    s8 ret,yl;
	LCD_Clear(WHITE);   //清屏
    if((ret = HTTP_POST(TEXT_Buffer))==0)
        printf("发送成功\r\n");
    else
        printf("发送失败\r\n");

    if(!(isSendDataError || isSendData) )
    {
        waitservice_flag=0;
        while(!(waitservice_flag ||(u2_data_Pack.USART2_RX_STA&(1<<15))));//等待服务器回应,如果大于
        
        
        if(AnalyticalData()!=0)
        {
            isSendDataError = 0;//设置标志位为成功
            printf("收到正确应答\r\n");
            if(sysData_Pack.data.failedTimes!=0)     //FLASH中存在未成功上传数据
            {
                for(yl=1; yl<=sysData_Pack.data.failedTimes; yl++) //分failedTimes上传服务器
                {
                    FLASH_GPS_Read(yl);     //读取FLASH未上传成功数据
                    if((ret = HTTP_POST(datatemp))==0)
                        printf("发送成功\r\n");
                    else
                        printf("发送失败\r\n");
                }
				         sysData_Pack.data.failedTimes=0;
            }
        }
        else
        {
            isSendDataError = 1;
            printf("未收到正确应答\r\n");
        }
    }
    ret = sendAT("AT+CIPCLOSE=1","OK",2000);	//关闭连接
    if(ret == 0)
    {
        printf("关闭连接成功\r\n");
    }
    else
    {
        printf("关闭连接失败\r\n");
    }

}


