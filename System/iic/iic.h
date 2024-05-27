 
#ifndef IIC_H_
#define IIC_H_

#include "stm32f4xx.h"

//IICʱ���߶�����PB6�������߶�����PB7
#define IIC_PORT          GPIOB
#define IIC_GPIO_CLOCK    RCC_AHB1Periph_GPIOB
#define IIC_SCL_PIN       GPIO_Pin_6
#define IIC_SDA_PIN       GPIO_Pin_7

void SCL_HIGH(void);          //SCL�����
void SCL_LOW(void);			  //SCL�����
void SDA_HIGH(void);          //SDA�����
void SDA_LOW(void);           //SDA�����
void SDA_IN(void);			  //SDA��Ϊ����ģʽ
u8 READ_SDA(void);            //��SDA����ŵ�ƽ

void IIC_Init(void);          //��ʼ��IIC
void IIC_Start(void);		  //����IIC��ʼ�ź�
void IIC_Stop(void);		  //����IICֹͣ�ź�
u8 IIC_Wait_Ack(void);        //IIC�ȴ�ACK�ź�
void IIC_Ack(void);           //IIC����ACK�ź�
void IIC_NAck(void);          //IIC������ACK�ź�
void IIC_Send_Byte(u8 txd);   //IIC����һ���ֽ�
u8 IIC_Read_Byte(u8 ack);     //IIC��ȡһ���ֽ�

#endif
