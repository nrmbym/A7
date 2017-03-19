#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "sys.h"

//59-63k �������δ�ϴ��ɹ��Ĵ������
//64K    ��ſ�������,������Ϣ����

//#define FLASH_SAVE_GPS     0X0800EC00       //�ӵ�59k��ʼ�����ڴ��δ�ܳɹ��ϴ���GPS��Ϣ��������һ���ϴ�
#define FLASH_SAVE_GPS     0X08010800        //65k��ʼ���
#define FLASH_SAVE_GPS_DEV 0X0800EC00+164   //ÿ��ƫ�ƹ̶��ĵ�ַ��strnlen()�������м���
#define GPS_END            0X08010000       //63k����ʼ��ַ
#define FLASH_Information  0X08010400       //64K��ʼ��ַ

//////////////////////////////////////////////////////////////////////////////////////////////////////
//�û������Լ�����Ҫ����
#define STM32_FLASH_SIZE 512 	 		//��ѡSTM32��FLASH������С(��λΪK)
#define STM32_FLASH_WREN 1              //ʹ��FLASHд��(0��������;1��ʹ��)
//////////////////////////////////////////////////////////////////////////////////////////////////////

//FLASH��ʼ��ַ
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH����ʼ��ַ
//FLASH������ֵ
u16 STMFLASH_ReadHalfWord(u32 faddr);		  //��������
void STMFLASH_WriteLenByte(u32 WriteAddr,u32 DataToWrite,u16 Len);	//ָ����ַ��ʼд��ָ�����ȵ�����
u32 STMFLASH_ReadLenByte(u32 ReadAddr,u16 Len);						//ָ����ַ��ʼ��ȡָ����������
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);		//��ָ����ַ��ʼд��ָ�����ȵ�����
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);   		//��ָ����ַ��ʼ����ָ�����ȵ�����

//����д��
void Test_Write(u32 WriteAddr,u16 WriteData);
extern char TEXT_Buffer[164];               //����������
extern char datatemp[164];                  //�����ȡ�������
extern void FLASH_initialize(void);         //FLASH�����ݵĳ�ʼ��
extern u8 signalQuality,old_signalQuality;
extern void FLASH_GPS_Pack(u8 m);    //δ�ϴ��ɹ��Ĵ�����ݴ�ŵ�FLASH��
extern void FLASH_GPS_Read(s8 j);    //FLASH�д�ŵ�GPS���ݵĶ�ȡ
extern void Erase_FLASH(u32 WriteAddr,u16 NumToWrite);     //����ָ����ַ��FLASH
extern void Testing_FLASH(u32 WriteAddr,u16 NumToWrite);   //����FLASH���Ƿ�Ϊ��
extern void saveData(void);                                //����д��FLASH
#endif

















