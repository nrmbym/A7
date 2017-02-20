#ifndef __USART_2_H
#define __USART_2_H
#include "sys.h"
#include "stdio.h"

#define USART2_MAX_RECV_LEN		5000	//�����ջ����ֽ���
#define USART2_MAX_SEND_LEN		2000	//����ͻ����ֽ���
#define USART2_RX_EN 			1		//0,������;1,����.
#define SRTING_NUM	 			20		//һ�������յ��ַ�������

extern void USART2_Init(u32 bound); //����2��ʼ������
void u2_printf(char* fmt,...);    //����2��ӡ����
#endif



