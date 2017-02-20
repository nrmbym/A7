#include "stmflash.h"
#include "delay.h"
#include "usart.h"
#include "usmat3.h"

char datatemp[165];            //������ʱ����FLASH�ж���������
char TEXT_Buffer[164];
u8 signalQuality,old_signalQuality;     //�ź�ǿ��,�Ժ���Ҫɾ������д


//��ȡָ����ַ�İ���(16λ����)
//faddr:����ַ(�˵�ַ����Ϊ2�ı���!!)
//����ֵ:��Ӧ����.
u16 STMFLASH_ReadHalfWord(u32 faddr)
{
    return *(vu16*)faddr;
}
#if STM32_FLASH_WREN	//���ʹ����д   
//������д��
//WriteAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToWrite:����(16λ)��  2���ֽڲ���
void STMFLASH_Write_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)
{
    u16 i;
    for(i=0; i<NumToWrite; i++)
    {
        FLASH_ProgramHalfWord(WriteAddr,pBuffer[i]);
        WriteAddr+=2;//��ַ����2.
    }
}
//��ָ����ַ��ʼд��ָ�����ȵ�����
//WriteAddr:��ʼ��ַ(�˵�ַ����Ϊ2�ı���!!)
//pBuffer:����ָ��
//NumToWrite:����(16λ)��(����Ҫд���16λ���ݵĸ���.)
#if STM32_FLASH_SIZE<256
#define STM_SECTOR_SIZE 1024 //�ֽ�
#else
#define STM_SECTOR_SIZE	2048
#endif
u16 STMFLASH_BUF[STM_SECTOR_SIZE/2];//�����2K�ֽ�
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)
{
    u32 secpos;	   //������ַ
    u16 secoff;	   //������ƫ�Ƶ�ַ(16λ�ּ���)
    u16 secremain; //������ʣ���ַ(16λ�ּ���)
    u16 i;
    u32 offaddr;   //ȥ��0X08000000��ĵ�ַ,��ʵ��ƫ�Ƶ�ַ
    if(WriteAddr<STM32_FLASH_BASE||(WriteAddr>=(STM32_FLASH_BASE+1024*STM32_FLASH_SIZE)))return;//�Ƿ���ַ
    FLASH_Unlock();						//����
    offaddr=WriteAddr-STM32_FLASH_BASE;		//ʵ��ƫ�Ƶ�ַ.
    secpos=offaddr/STM_SECTOR_SIZE;			  //������ַ  0~127 for STM32F103RBT6
    secoff=(offaddr%STM_SECTOR_SIZE)/2;		//�������ڵ�ƫ��(2���ֽ�Ϊ������λ.)
    secremain=STM_SECTOR_SIZE/2-secoff;		//����ʣ��ռ��С
    if(NumToWrite<=secremain)secremain=NumToWrite;//�����ڸ�������Χ����д������ݳ��ȸ���ʣ��Ŀռ�����
    while(1)
    {
        STMFLASH_Read(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//������������������
        for(i=0; i<secremain; i++) //У������
        {
            if(STMFLASH_BUF[secoff+i]!=0XFFFF)break;//��Ҫ����
        }
        if(i<secremain)//��Ҫ����
        {
            FLASH_ErasePage(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE);//�����������
            for(i=0; i<secremain; i++) //����
            {
                STMFLASH_BUF[i+secoff]=pBuffer[i];
            }
            STMFLASH_Write_NoCheck(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//д����������
        } else STMFLASH_Write_NoCheck(WriteAddr,pBuffer,secremain);//д�Ѿ������˵�,ֱ��д������ʣ������.
        if(NumToWrite==secremain)break;//д�������          ʵ�ʵ���������д������ѭ�������break
        else//д��δ����
        {
            secpos++;				//������ַ��1
            secoff=0;				//ƫ��λ��Ϊ0 	 ��Ϊ����һ���µ�ҳ������ָ���ƫ�Ƶĵ�ַ��0
            pBuffer+=secremain;  	//ָ��ƫ��
            WriteAddr+=secremain;	//д��ַƫ��
            NumToWrite-=secremain;	//�ֽ�(16λ)���ݼ�
            if(NumToWrite>(STM_SECTOR_SIZE/2))secremain=STM_SECTOR_SIZE/2;//��һ����������д����
            else secremain=NumToWrite;//��һ����������д����
        }
    };
    FLASH_Lock();//����
}
#endif

//��ָ����ַ��ʼ����ָ�����ȵ�����
//ReadAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToWrite:����(16λ)��
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead)
{
    u16 i;
    for(i=0; i<NumToRead; i++)
    {
        pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr);//��ȡ2���ֽ�.
        ReadAddr+=2;//ƫ��2���ֽ�.
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//WriteAddr:��ʼ��ַ
//WriteData:Ҫд�������
void Test_Write(u32 WriteAddr,u16 WriteData)
{
    STMFLASH_Write(WriteAddr,&WriteData,1);//д��һ����
}

void saveData(void)
{
    CRC_ResetDR();//��λCRC
    sysData_Pack.data.CRCData = CRC_CalcBlockCRC((u32*)sysData_Pack.buf, sizeof(_sysData)/4 - 2);  //����CRC   ������һ�ε�CRCУ��
    sysData_Pack.data.isEffective = 1;//��־λ
    STMFLASH_Write(FLASH_Information,sysData_Pack.buf,sizeof(_sysData)/2);//������д��FLASH	 ����2��ԭ����STMFLASH_Write()Ҫ����16λ���ֽ�
}


void FLASH_initialize(void)  //������ʼ��FLASH������
{
    u32 CRC_DATA = 0;
    STMFLASH_Read(FLASH_Information,sysData_Pack.buf,sizeof(_sysData)/2);    //��FLASH�е����ݶ�ȡ����
    if(sysData_Pack.data.isEffective == 1)    //FLASH��������Ч
    {
        CRC_ResetDR();         //��λCRC���ݼĴ���
        //�����ĺ��� 1.ָ�����Ҫ��������ݵĻ�������ָ��.  2.Ҫ����Ļ������ĳ���
        CRC_DATA = CRC_CalcBlockCRC((u32*)sysData_Pack.buf, sizeof(_sysData)/4 - 2);   //CRC����У�� ÿһ������4���ֽڣ��õ�����,��ȥ����ġ��õ�У�����ݳ���
        if(CRC_DATA == sysData_Pack.data.CRCData)    //У��λ��Ч
        {
            sysData_Pack.data.bootTimes++;         //��¼���������Ĵ���
        }
        else
        {
            sysData_Pack.data.isEffective = 0;     //���FLASH��������Ч
        }
    }

    if(sysData_Pack.data.isEffective != 1)    //FLASH�е�������Ч,��ʼ������
    {
        sysData_Pack.data.bootTimes = 1;                               //��������
        sysData_Pack.data.submitInformationErrorTimes = 0;             //�ϴ�������ʧ�ܴ���
        sysData_Pack.data.postErrorTimes = 0;                          //POSTʧ�ܴ���
        sysData_Pack.data.GPSErrorTimes = 0;                           //��ΪGPSδ��λʱ�䳬ʱ�����������
        sysData_Pack.data.isEffective = 1;                             //��¼FLASH�е������Ƿ���Ч
    }
    saveData();//������д��FLASH

    printf("�豸����������%d\r\n",sysData_Pack.data.bootTimes);
    printf("�ϴ�������ʧ�ܴ�����%d\r\n",sysData_Pack.data.submitInformationErrorTimes);
    printf("postErrorTimes��%d\r\n",sysData_Pack.data.postErrorTimes);
    printf("GPS��ʱδ��λ������%d\r\n\r\n",sysData_Pack.data.GPSErrorTimes);
}



/********************************************
FLASH�� 59K��63K���GPSδ�ϴ��ɹ�������
1K���6������
���Դ��30��δ�ɹ��ϴ���GPS����
*********************************************/
void FLASH_GPS_Pack(u8 m)   //δ�ϴ��ɹ��Ĵ�����ݴ�ŵ�FLASH��
{ 
	 //д֮ǰ�ȶ���������Ƿ�������(δ��ɵĹ���)
	 //��������û���ϴ��ɹ�
	 //����û���ϴ��ɹ� ����һ��i��ȫ�ֱ�������¼�м���δ�ɹ��ϴ�
	 //ÿ�ιػ�ʱ,��������ݷ���FLASH��,����ʱ�����ȡ�ϴ�
    if((m+1)*Pack_length<=Pack_length*6)    //59k���
    {
        STMFLASH_Write(FLASH_SAVE_GPS+m*Pack_length,(u16*)TEXT_Buffer,sizeof(TEXT_Buffer)/2);    //д��GPS���ݵ�FLASH
    }
    else if(5<m&&m<=11)     //60k
    {
        STMFLASH_Write(FLASH_SAVE_GPS+1024+(m-6)*Pack_length,(u16*)TEXT_Buffer,sizeof(TEXT_Buffer)/2);
    }
    else if(11<m&&m<=17)    //61
    {
        STMFLASH_Write(FLASH_SAVE_GPS+1024*2+(m-12)*Pack_length,(u16*)TEXT_Buffer,sizeof(TEXT_Buffer)/2);
    }
    else if(11<m&&m<=17)    //62
    {
        STMFLASH_Write(FLASH_SAVE_GPS+1024*3+(m-18)*Pack_length,(u16*)TEXT_Buffer,sizeof(TEXT_Buffer)/2);
    }
    else                    //63
    {
        STMFLASH_Write(FLASH_SAVE_GPS+1024*4+(m-24)*Pack_length,(u16*)TEXT_Buffer,sizeof(TEXT_Buffer)/2);
    }
		printf("δ�ϴ��ɹ���GPS�����ѱ���FLASH\r\n\r\n");
    m++;
    if(m>=30)   //���������Ĵ洢�ռ�,�ӵ�һ����ʼ���´��
    {
        m=0;
    }
}

void FLASH_GPS_Read(s8 j)
{
    if((j+1)*Pack_length<=(Pack_length*6))  //��һ���ֽ���
    {
        STMFLASH_Read(FLASH_SAVE_GPS+j*Pack_length,(u16*)datatemp,sizeof(TEXT_Buffer)/2);
        datatemp[Pack_length]='\0';
        printf("%s\r\n",datatemp);
    }
    else if(5<j&&j<=11)   //2
    {
        STMFLASH_Read(FLASH_SAVE_GPS+1024+(j-6)*Pack_length,(u16*)datatemp,sizeof(TEXT_Buffer)/2);
        datatemp[Pack_length]='\0';
        printf("%s\r\n",datatemp);
    }
    else if(11<j&&j<=17)    //3
    {
        STMFLASH_Read(FLASH_SAVE_GPS+1024*2+(j-12)*Pack_length,(u16*)datatemp,sizeof(TEXT_Buffer)/2);
        datatemp[Pack_length]='\0';
        printf("%s\r\n",datatemp);
    }
    else if(11<j&&j<=17)  //4
    {
        STMFLASH_Read(FLASH_SAVE_GPS+1024*3+(j-18)*Pack_length,(u16*)datatemp,sizeof(TEXT_Buffer)/2);
        datatemp[Pack_length]='\0';
        printf("%s\r\n",datatemp);
    }
    else          //5�ֽ�
    {
        STMFLASH_Read(FLASH_SAVE_GPS+1024*4+(j-24)*Pack_length,(u16*)datatemp,sizeof(TEXT_Buffer)/2);
        datatemp[Pack_length]='\0';
        printf("%s\r\n",datatemp);
    }
    j++;
    if(j>=30)   //���������Ĵ洢�ռ�,�ӵ�һ����ʼ���´��
    {
        j=0;
    }
}

//��FLASHָ��λ�õĲ���
//ֻ��һҳһҳ�Ĳ���
//������� WriteAddr      ��Ҫ�����ĵ�ַ
//         NumToWrite     ��Ҫ�����İ���(16λ)��
void Erase_FLASH(u32 WriteAddr,u16 NumToWrite)
{
    u32 secpos;	   //������ַ
    u16 secoff;	   //������ƫ�Ƶ�ַ(16λ�ּ���)
    u16 secremain; //������ʣ���ַ(16λ�ּ���)
    u16 i;
    u32 offaddr;   //ȥ��0X08000000��ĵ�ַ
    if(WriteAddr<STM32_FLASH_BASE||(WriteAddr>=(STM32_FLASH_BASE+1024*STM32_FLASH_SIZE)))return;//�Ƿ���ַ
    FLASH_Unlock();						//����
    offaddr=WriteAddr-STM32_FLASH_BASE;		//ʵ��ƫ�Ƶ�ַ.
    secpos=offaddr/STM_SECTOR_SIZE;			//������ַ  0~127 for STM32F103RBT6
    secoff=(offaddr%STM_SECTOR_SIZE)/2;		//�������ڵ�ƫ��(2���ֽ�Ϊ������λ.)
    secremain=STM_SECTOR_SIZE/2-secoff;		//����ʣ��ռ��С
    if(NumToWrite<=secremain)secremain=NumToWrite;//�����ڸ�������Χ
    while(1)
    {
        STMFLASH_Read(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//������������������
        for(i=0; i<secremain; i++) //У������
        {
            if(STMFLASH_BUF[secoff+i]!=0XFFFF)break;//��Ҫ����
        }
				
        if(i<secremain)//��Ҫ����
        {
            FLASH_ErasePage(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE);//�����������
        }
        break;
    };
    FLASH_Lock();//����
}


//���FLASH�������Ƿ�Ϊ��
//������� WriteAddr     ��Ҫ���ĵ�ַ
//         NumToWrite   ��Ҫ���İ���(16λ)��
void Testing_FLASH(u32 WriteAddr,u16 NumToWrite)
{
    u32 secpos;	   //������ַ
    u16 secoff;	   //������ƫ�Ƶ�ַ(16λ�ּ���)
    u16 secremain; //������ʣ���ַ(16λ�ּ���)
    u16 i;
    u32 offaddr;   //ȥ��0X08000000��ĵ�ַ,��ʵ��ƫ�Ƶ�ַ
    if(WriteAddr<STM32_FLASH_BASE||(WriteAddr>=(STM32_FLASH_BASE+1024*STM32_FLASH_SIZE)))return;//�Ƿ���ַ
    FLASH_Unlock();						//����
    offaddr=WriteAddr-STM32_FLASH_BASE;		//ʵ��ƫ�Ƶ�ַ.
    secpos=offaddr/STM_SECTOR_SIZE;			  //������ַ  0~127 for STM32F103RBT6
    secoff=(offaddr%STM_SECTOR_SIZE)/2;		//�������ڵ�ƫ��(2���ֽ�Ϊ������λ.)
    secremain=STM_SECTOR_SIZE/2-secoff;		//����ʣ��ռ��С
    if(NumToWrite<=secremain)secremain=NumToWrite;//�����ڸ�������Χ����д������ݳ��ȸ���ʣ��Ŀռ�����
    while(1)
    {
        STMFLASH_Read(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//������������������
        for(i=0; i<secremain; i++) //У������
        {
            if(STMFLASH_BUF[secoff+i]!=0XFFFF)break; 
        }
        if(i<secremain)  //��Ϊ��
        {
					 printf("�����FLASH��Ϊ��\r\n"); break;
        }
        else
				{
					printf("�����FLASH��\r\n"); break;
				}			
    };
    FLASH_Lock();//����
}









