#include "iic.h"

#include "delay.h"

/**
  * @brief  SCL����ߵ�ƽ����
  * @param  None
  * @retval None
  */
void SCL_HIGH(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(IIC_GPIO_CLOCK, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = IIC_SCL_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
	GPIO_Init(IIC_PORT, &GPIO_InitStructure);
	
	GPIO_SetBits(IIC_PORT, IIC_SCL_PIN);
}

void SCL_LOW(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(IIC_GPIO_CLOCK, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = IIC_SCL_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
	GPIO_Init(IIC_PORT, &GPIO_InitStructure);
	
	GPIO_ResetBits(IIC_PORT, IIC_SCL_PIN);
}



void SDA_HIGH(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(IIC_GPIO_CLOCK, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = IIC_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
	GPIO_Init(IIC_PORT, &GPIO_InitStructure);
	
	GPIO_SetBits(IIC_PORT, IIC_SDA_PIN);
}

void SDA_LOW(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(IIC_GPIO_CLOCK, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = IIC_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
	GPIO_Init(IIC_PORT, &GPIO_InitStructure);
	
	GPIO_ResetBits(IIC_PORT, IIC_SDA_PIN);
}

void SDA_IN(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(IIC_GPIO_CLOCK, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = IIC_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(IIC_PORT, &GPIO_InitStructure);
}

u8 READ_SDA(void)
{
	SDA_IN();
	
	return GPIO_ReadInputDataBit(IIC_PORT, IIC_SDA_PIN);
}


void IIC_Init(void)
{
	SCL_HIGH();
	SDA_HIGH();     //SCL��SDA��ʼ�����, ׼��״̬
}

void IIC_Start(void)
{
	SDA_HIGH();
	SCL_HIGH();
	delay_us(4);
	SDA_LOW();      //START:when CLK is high,DATA change form high to low 
	delay_us(4);
	SCL_LOW();      //ǦסIIC���ߣ�׼�����ͻ��������
}


void IIC_Stop(void)
{
	SCL_LOW();
	SDA_LOW();      //STOP:when CLK is high DATA change form low to high
	delay_us(4);
	SCL_HIGH();
	SDA_HIGH();     //����IIC���߽����ź�
	delay_us(4);
}

u8 IIC_Wait_Ack(void)
{
	u8 ucErrTime = 0;
	SDA_IN();        //SDA����Ϊ����
	delay_us(1);
	SCL_HIGH();
	delay_us(1);
	while(READ_SDA())
	{
		ucErrTime++;
		if(ucErrTime > 250)
		{
			IIC_Stop();
			return 1;
		}
	}
	SCL_LOW();
	return 0;
}

void IIC_Ack(void)
{
	SCL_LOW();
	SDA_LOW();
	delay_us(2);
	SCL_HIGH();
	delay_us(2);
	SCL_LOW();
}

void IIC_NAck(void)
{
	SCL_LOW();
	SDA_HIGH();
	delay_us(2);
	SCL_HIGH();
	delay_us(2);
	SCL_LOW();
}

/**
  * @brief  IIC����һ���ֽں���
  * @param  txd: ��Ҫ���͵�һ���ֽ�
  * @retval None
  */
void IIC_Send_Byte(u8 txd)
{
	u8 t;
	SCL_LOW();              //����ʱ�ӿ�ʼ���ݴ���
	for(t = 0; t < 8; t++)
	{
		if(txd & 0x80)
			SDA_HIGH();
		else
			SDA_LOW();
		txd<<=1;
		delay_us(2);
		SCL_HIGH();
		delay_us(2);
		SCL_LOW();
		delay_us(2);
	}
}

/**
  * @brief  IIC��ȡһ���ֽں���
  * @param  ack: 1, Ӧ��
                 0, ��Ӧ��
  * @retval ��ȡ��һ���ֽ�
  */
u8 IIC_Read_Byte(u8 ack)
{
	u8 i, receive = 0;
	SDA_IN();                 //SDA����Ϊ����
	for(i = 0; i < 8; i++)
	{
		SCL_LOW();
		delay_us(2);
		SCL_HIGH();
		receive<<=1;
		if(READ_SDA())
			receive++;
		delay_us(1);
	}
	if(!ack)
		IIC_NAck();
	else
		IIC_Ack();
	return receive;
}












