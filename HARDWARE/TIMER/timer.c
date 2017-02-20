#include "timer.h"
#include "usmat3.h"
#include "stdio.h"
#include "stmflash.h"
#include "iwdg.h"
#include "conversion.h"
#include "a7.h"
u32 GPS_Upload=0;    //GPS数据打包标志
double 	LatNow = 0,LonNow = 0,LatOld=0,LonOld=0;//记录上传时的纬度
u32 GPS_invalidtimes=0;
u8 Error=0;					//操作808是否出错。0:未出错 1:出错
u32 Upload_Time=0;  //上传服务器时间间隔
u32 Heartbeat_Upload=0;  //心跳上传标志位
u8 submit_info_flag=0;   //发送GPS标志位、、
u8 GPS_PACK=0;           //两次打包完成标志位
u8 waitservice_flag=0;   //等待服务器超时标志
u8 LEDshine_flag=0;     
//通用定时器3中断初始化
//这里时钟选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数


void TIM3_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能

    //定时器TIM3初始化
    TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
    TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位

    TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

    //中断优先级NVIC设置
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //先占优先级0级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  //从优先级3级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
    NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器

    TIM_Cmd(TIM3, ENABLE);  //使能TIMx
}

//定时器3中断服务程序
//每100ms进一次中断服务函数
void TIM3_IRQHandler(void)
{
    static u32 times=0;
    static u8 tt=0;
    static u32 waitservicetime = 0;  //服务器等待的计时
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)    //是更新中断
    {
        TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除TIMx更新中断标志
        times++;

        if(Error==0)   //没有出错
        {
            IWDG_Feed(); //喂狗
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
        case 5:    //正常工作状态 gps打开
        {
			if(times%10== 0)  //每一秒闪一次灯
			{
				LEDshine_flag=1;
			}
            if(submit_info_flag==0)
            {
                tt++;
                if(tt%300==0)   //每30秒上传一次
                {
                    submit_info_flag=1;
                }
            }

            if(times%100==0)       //每正常工作1s
            {
                if(GPS_effective==1)    //GPS数据有效
                {
                    GPS_effective=0;    //GPS数据标记为无效
                    if(times%15==0)     //每15s输出一次滤波信息
                    {
                        //只有前后两次的距离超过40m才保证打包数据
                        getFilterLoc(&LatNow, &LonNow);   //得到滤波后的经纬度  把两个存储变量的地址传入，赋给一个指针变量
                        printf("滤波后的百度地图纬度%f,经度%f\r\n",LatNow,LonNow);
                        distance=Cal_Distance(LatOld,LonOld,LatNow,LonNow)*1000;       //计算两次定位的   (第一次定位的距离差距很大)
                        printf("\r\n两次上传的距离为%lfm\r\n",distance);
                        LatOld=LatNow;           //便于下一次计算距离
                        LonOld=LonNow;
//											if((40<distance)&&(distance<1000))     //只有当距离满足时,才会打包
                        {
                            GPS_Upload=1;  			 //GPS打包标志位
                        }
                    }
                    //GPS信息出现有效时 切换回GPS
                    //							if()   //区分是什么什么定位信息  AGPS  GPS
                    //							{
                    //								//关闭AGPS
                    //							}

                    GPS_invalidtimes=0;   //GPS未定位的时间清0
                }
//								else
//								{
//										GPS_invalidtimes++;
//										printf("未成功定位的时间%d\r\n\r\n",GPS_invalidtimes);
//										if(GPS_invalidtimes>=GPS_Switch_AGPS)   //超过30s收到GPS数据
//										{
//												//只有在有GPS时转到AGPS时才进入
//												printf("启动AGPS数据网络定位\r\n\r\n");
//												AGPS_Location();

//										}
//										if(GPS_invalidtimes>=GPSFailedTime_MAX)  //超过5分钟没有定位成功
//										{
//												printf("GPS定位超时\r\n\r\n");
//												sysData_Pack.data.GPSErrorTimes++;
//												saveData();//将数据写入FLASH
//												GPS_invalidtimes=0;          //未定位时间清0   没有清0的话保存的数据不对
//		//                    Error=1;   //标志看门狗，等待复位
//										}
//								}

//								Upload_Time++;   //上传时间间隔

//								if(Upload_Time>=Upload_Time_MAX)    //超过5分钟没有上传
//								{
//										Heartbeat_Upload=1;   //心跳上传标志
//								}

//								if(Upload_Time>=Upload_Time_MAX+120)    //超过6分钟没有上传
//								{
//										//证明心跳没有上传成功
////										Error=1;   //标志看门狗，等待复位
//								}

            }
            if(waitservice_flag==0)// 标志位如果被标记为等待，进行计时等待
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





















//定时器7中断服务程序
void TIM7_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)//是更新中断
    {
        u2_data_Pack.USART2_RX_STA|=1<<15;	//标记接收完成
        TIM_ClearITPendingBit(TIM7, TIM_IT_Update  );  //清除TIM7更新中断标志
        TIM_Cmd(TIM7, DISABLE);  //关闭TIM7
    }
}

//通用定时器7中断初始化
//这里时钟选择为APB1的2倍，而APB1为42M
//arr：自动重装值。
//psc：时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=定时器工作频率,单位:Mhz
//通用定时器中断初始化
//这里始终选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数
void TIM7_Int_Init(u16 arr,u16 psc)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);//TIM7时钟使能

    //定时器TIM7初始化
    TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
    TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
    TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位

    TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE ); //使能指定的TIM7中断,允许更新中断

    TIM_Cmd(TIM7,ENABLE);//开启定时器7

    NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;//抢占优先级0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//子优先级2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

}







