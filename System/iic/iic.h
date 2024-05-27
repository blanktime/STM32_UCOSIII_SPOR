 
#ifndef IIC_H_
#define IIC_H_

#include "stm32f4xx.h"

//IIC时钟线定义在PB6，数据线定义在PB7
#define IIC_PORT          GPIOB
#define IIC_GPIO_CLOCK    RCC_AHB1Periph_GPIOB
#define IIC_SCL_PIN       GPIO_Pin_6
#define IIC_SDA_PIN       GPIO_Pin_7

void SCL_HIGH(void);          //SCL输出高
void SCL_LOW(void);			  //SCL输出低
void SDA_HIGH(void);          //SDA输出高
void SDA_LOW(void);           //SDA输出低
void SDA_IN(void);			  //SDA变为输入模式
u8 READ_SDA(void);            //读SDA输入脚电平

void IIC_Init(void);          //初始化IIC
void IIC_Start(void);		  //发送IIC起始信号
void IIC_Stop(void);		  //发送IIC停止信号
u8 IIC_Wait_Ack(void);        //IIC等待ACK信号
void IIC_Ack(void);           //IIC发送ACK信号
void IIC_NAck(void);          //IIC不发送ACK信号
void IIC_Send_Byte(u8 txd);   //IIC发送一个字节
u8 IIC_Read_Byte(u8 ack);     //IIC读取一个字节

#endif
