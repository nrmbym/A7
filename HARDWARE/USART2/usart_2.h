#ifndef __USART_2_H
#define __USART_2_H
#include "sys.h"
#include "stdio.h"

#define USART2_MAX_RECV_LEN		5000	//最大接收缓存字节数
#define USART2_MAX_SEND_LEN		2000	//最大发送缓存字节数
#define USART2_RX_EN 			1		//0,不接收;1,接收.
#define SRTING_NUM	 			20		//一次最多接收的字符串数量

extern void USART2_Init(u32 bound); //串口2初始化函数
void u2_printf(char* fmt,...);    //串口2打印函数
#endif



