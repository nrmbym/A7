#include "delay.h"
#include "usart_2.h"
#include "stdarg.h"	 	  	 
#include "string.h"	 
#include "timer.h"
#include "A7.h"

void USART2_Init(u32 bound)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	// GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE); //串口2时钟使能

	USART_DeInit(USART2);  //复位串口2
	//USART2_TX   PA2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA2
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA2

	//USART2_RX	  PA3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;//PA3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA3

	USART_InitStructure.USART_BaudRate = bound;//波特率一般设置为115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

	USART_Init(USART2, &USART_InitStructure); //初始化串口2


	USART_Cmd(USART2, ENABLE);                    //使能串口 

	//使能接收中断
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); 
	//开启ERR中断
	USART_ITConfig(USART2, USART_IT_ERR, ENABLE);	
	//关闭发送中断
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE); 

	//设置中断优先级
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	
	
	
	TIM7_Int_Init(99,7199);		//10ms中断
	u2_data_Pack.USART2_RX_STA=0;		//清零
	TIM_Cmd(TIM7,DISABLE);			//关闭定时器7
}

//串口2中断服务函数
//将接收到的数据存入接收缓存中，并将字符串分离
//可以判读是否收到GPS数据
void USART2_IRQHandler(void)
{
	u8 res;	      
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)//接收到数据
	{	 
		res =USART_ReceiveData(USART2);		 
		if((u2_data_Pack.USART2_RX_STA&(1<<15))==0)//接收完的一批数据,还没有被处理,则不再接收其他数据
		{ 
			if(u2_data_Pack.USART2_RX_STA<USART2_MAX_RECV_LEN)	//还可以接收数据
			{
				TIM_SetCounter(TIM7,0);//计数器清空          				//计数器清空
				if(u2_data_Pack.USART2_RX_STA==0) 				//使能定时器7的中断 
				{
					TIM_Cmd(TIM7,ENABLE);//使能定时器7
				}
				u2_data_Pack.USART2_RX_BUF[u2_data_Pack.USART2_RX_STA++]=res;	//记录接收到的值	 
			}else 
			{
				u2_data_Pack.USART2_RX_STA|=1<<15;				//强制标记接收完成
			} 
		}
	}  	
	
	//溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题
    if(USART_GetFlagStatus(USART2,USART_FLAG_ORE) == SET)
    {
        USART_ClearFlag(USART2,USART_FLAG_ORE);	//读SR
        USART_ReceiveData(USART2);				//读DR
    }
}  
//串口2,printf 函数
//确保一次发送数据不超过USART2_MAX_SEND_LEN字节
void u2_printf(char* fmt,...)
{  
	u16 i,j; 
	va_list ap; 
	va_start(ap,fmt);
	vsprintf((char*)u2_data_Pack.USART2_TX_BUF,fmt,ap);
	va_end(ap);
	i=strlen((const char*)u2_data_Pack.USART2_TX_BUF);		//此次发送数据的长度
	for(j=0;j<i;j++)							//循环发送数据
	{
		while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET); //循环发送,直到发送完毕   
		USART_SendData(USART2,u2_data_Pack.USART2_TX_BUF[j]); 
	} 
}
