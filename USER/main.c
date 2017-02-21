#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "usart.h"
#include "usmat3.h"
#include "timer.h"
#include "stmflash.h"
#include "iwdg.h"
#include "usart_2.h"
#include "A7.h"
#include "lcd.h"
/****************************************************
实验室stm32开发板按键备注
开发板原理图上的按键备注是错的  其余的标注按键都没有
红色按键        系统复位按键
中间按键        KEY0_PRES 对应管脚PE4  KEY0
最后的一个按键  WKUP_PRES 对应管脚PA0  WKUP

使用芯片 stm32F103xb  使用的是中等容量芯片
1.CORE中的启动文件 需要更改为中等容量的启动文件
2.Define 中的设置需要更改
3.FLASH的操作 芯片一页是1k

hd(High Density )是大容量，
md(Medium Density ) 是中容量
ld (Low Density ) 是小容量
****************************************************/



#define BaudRate_usart3 9600        //串口3的波特率(获取A7返回的GPS数据)
#define BaudRate_usart2 115200	    //串口AT指令发送和接收

int main(void)
{
	  u8 key;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
    IWDG_Init(6,156);         //初始化看看门狗 溢出时间为1s
    delay_init();	    	      //延时函数初始化
    delay_ms(100);            //等待电源稳定

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE);//开启CRC时钟CRC校验
    uart_init(115200);	      //串口初始化为115200
    TIM3_Int_Init(999,7199);  //初始化定时器 100ms
    USART2_Init(BaudRate_usart2);
    LCD_Init();
    LCD_Display_Dir(1);
    FLASH_initialize();       //初始化FLASH中第64k的数据记录
    LED_Init();		  	        //初始化与LED连接的硬件接口
    KEY_Init();               //按键初始化
    A7_Init();                //A7模块初始化
    USART3_Init(BaudRate_usart3);    //初始化串口3 用来接收GPS数据
	
    while(1)
    {
			key=KEY_Scan(0);
        if(GPS_Upload==1)   //GPS数据打包标志
        {
            GPS_Packed_Data();
        }
        if((submit_info_flag==1)&&(GPS_PACK==1))  //当距离上一次发送的定时到了，并且已经是打包完成，才进行发送
        {
            submit_info_flag=0;
            A7_SendPost();          //数据上传服务器
//            if(isSendDataError==1)  //没有上传成功
					if(key==4)
            {
                failedTimes++;       //提交服务器失败次数
                Post_Errotimes++;
                FLASH_GPS_Pack(failedTimes);    //未上传成功存放到FLASH中
            }
        }
        if(Post_Errotimes>Post_Errotimes_MAX)
        {
            A7_Init();
        }
        if(Heartbeat_Upload==1)  //上传心跳
        {
            //这里缺少上传心跳函数
            //上传成功 Upload_Time=0; 才清0
            Heartbeat_Upload=0;   //标志位清0
        }
        if(LEDshine_flag ==1 )
        {
            LEDshine_flag=0;
            LED1=~LED1;
            LED0=~LED0;
        }
    }
}



/*
问题记录
1.A6模块开机不稳定，AT指令初始化不稳定(解决)
2.AT指令<的识别不到的问题 (好像是末尾没有\r\n不能识别。目前还不确定) (解决GPS和AT指令的串口分开)
3.GPS数据解析(解决)
4.10个GPS数据滤波储存队列的方法  （已确定）
5.FLSH中数据存储。打包GPS数据存储和开机错误次数储存是否分开(用1k错误次数分开存放)
是否记录出现错误时，是哪个部分的错误次数(模块,AT指令,电源,单片机)
6.串口2接收函数的改动 (目前基本稳定)
7.GPS真实坐标转百度坐标位置是否正确(基本准确)
8.计算推荐定位信息的校验和有问题。 定位信号强度要打印出来(信号强度可以通过软件查看)
9.GPS数据偏移高德地图(高德地图不支持坐标查询)
10.对FLASH的操作(数据的存放为什么要用联合体)  __packed可以避免自动对齐,可以用来压缩数据，让FLASH存放得更多的数据
11.FLASH中保存数据没有进行CRC校验  (已进行)
12.FLASH30个数据存放更新的问题(上传成功就清除FLASH中所有的数据)
通过检测FLASH中是否有数据来确定是否上传
13.两个GPS数据返回时，时间是一样的。(已解决,原因标志位没有清除)
14.FLASH数据存储时算法优化.怎么实现进行排序(不用进行排序,有时间在数据里面即可)
15.AGPS定位数据(辅助定位信息),在GPS信号不好时，切换到AGPS进行辅助定位。信号好的时候再切换回来
定位技术基本包含基于GPS的定位和基于蜂窝基站的定位两类
GSM蜂窝网络为例，一般是通过GPRS网络为辅助工具

在AGPS中，通过从蜂窝网络下载当前地区的可用卫星信息（包含当地区可用的卫星频段、方位、仰角等信息），从而避免了全频段大范围搜索，使首次搜星速度大大提高，时间由原来的几分钟减小到几秒钟。
如上图所示,AGPS中从定位启动到GPS接收器找到可用卫星的基本流程如下：
（1）设备从蜂窝基站获取到当前所在的小区位置（即一次COO定位）
（2）设备通过蜂窝网络将当前蜂窝小区位置传送给网络中的AGPS位置服务器
（3）APGS位置服务器根据当前小区位置查询该区域当前可用的卫星信息（包括卫星的频段、方位、仰角等相关信息），并返回给设备
（4）GPS接收器根据得到的可用卫星信息，可以快速找到当前可用的GPS卫星
至此，GPS接收器已经可正常接收GPS信号，GPS初始化过程结束。AGPS对定位速度的提高就主要体现在此过程中.

SIM808返回的数据
基站的信息(需要第3方软件解析。有开放的API接口函数)
+CENG: 0,"460,00,3340,624b,19,36"
+CENG: 1,"460,00,3340,5e8a,15,41"
+CENG: 2,"460,00,3340,5e62,25,34"
+CENG: 3,"460,00,3340,6249,06,34"
+CENG: 4,"460,00,3340,5e61,05,32"
+CENG: 5,"460,00,3340,61c7,20,27"
+CENG: 6,"460,00,3340,5e63,14,26"
其中返回的基站位置信息：0-当前小区，1-6-邻近小区
移动联通基站查询定位
MCC 460
MNC 00
(后面的是输入16进制)
LAC 3340
CELL 624b

SIM900
AGPS返回的直接带GPS数据

安信可A7模块
1.可以控制GPS数据返回的速度
2.支持AGP  Sat+agps=1 //打开 agps，本条指令由于要联网下载星历数据，周期会比较长。返回的信息直接带GPS信息
AGPS存在定位不准的情况

16.在进行切换时,GPS信息超过20s都没有返回时，切换到AGPS数据
获取的GPS信息1s返回一条

17.数据打包上传服务器时，两次定位距离超过40M以上才上传,并且和上次上传的时间有一定的时间间隔。
定位信息基本没有动时,一定时间上传心跳信息,查看设备是否存活。

18.FLSH中存放的数据,上传成功后就要擦除。
FLASH操作函数需要更改(2k改1k)

19.怎么确定什么时候上传心跳信息(心跳信息里面带有当前的位置的GPS数据。上传失败，不保存在FLASH中)

20.AGPS在什么时候关闭。通过算相邻的两个有效的信息超过很大的时候关闭AGPS.(在两次即时数据中,距离差距很大,说明GPS和AGPS同时存在),
此时就可以关闭AGPS，GPS数据的返回单独开一个串口接收

21.无符型不用考虑变量的溢出，溢出后是直接回到0

22.最大问题，程序逻辑性稳定问题

23.GPS数据开关问题   上传时可以不关接收

24.汽车停在一个一直不能定位的地方怎么办

25.AGPS需要很久定位时间。或者是说在室内就不能定位

*/

