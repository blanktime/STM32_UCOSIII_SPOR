#include <string.h>
#include <stdint.h>  // For uint8_t and intptr_t
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "os.h"      // uC/OS-III header file
#include "stm32f4xx.h"
#include "gpiof.h"
#include "delay.h"
#include "uart.h"

#define MAX_KEYWORDS 2
#define QUEUE_SIZE 512

//#define rw_spor
#define fix_spor
//#define random_spor
//#define progress_spor
#define detect_word

volatile int keywordDetected = 0;

const char *TARGET_KEYWORDS[MAX_KEYWORDS] = {
    "ufs_mtk_abort_handler",
//    "ufshcd_abort",
    "Host UIC Error Code",
//    "abort: tag",
//    "ufshcd_wait_for_dev_cmd",
//    "Query Request timeout",
//    "ERR! intr_status",
//    "ufshcd_probe_hba failed"
};

// Arrays to hold KMP partial match tables and keyword lengths
int *kmpTables[MAX_KEYWORDS];
int keywordLengths[MAX_KEYWORDS];

// Queue for USART data
OS_Q usartQueue;  

//�������ȼ�
#define START_TASK_PRIO		3
//�����ջ��С	
#define START_STK_SIZE 		512
//������ƿ�
OS_TCB StartTaskTCB;
//�����ջ	
CPU_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *p_arg);

// Task Detect Prio
#define TASK_DETECT_PRIO	4
// Task detect Stk Size
#define TASK_DETECT_STK_SIZE 	128
// Task Control Blocks for detection tasks
OS_TCB TaskDetectTCB[MAX_KEYWORDS];  
// Stacks for detection tasks
CPU_STK TaskDetectStk[MAX_KEYWORDS][TASK_DETECT_STK_SIZE];  
// Task func
void TaskDetect_task(void *p_arg);

// SPOR Prio
#define SPOR_PRIO	4
// SPOR Stk Size
#define SPOR_STK_SIZE 	128
// Task Control Blocks for SPOR task
OS_TCB SPORTaskTCB;  
// Stacks for SPOR task
CPU_STK SPOR_Stk[SPOR_STK_SIZE];  
// Task func
void SPOR_task(void *p_arg);

void buildKMPTables(void);
void KMP(char *keyword, const char *pattern, int *kmpTable, int keywordLength);

int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	
	delay_init(168);  	//ʱ�ӳ�ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�жϷ�������
	uart_init(115200);  //���ڳ�ʼ��
    
    OSInit(&err);		//��ʼ��UCOSIII
    
    // Create the USART data queue
    OSQCreate((OS_Q*		)&usartQueue,       //��Ϣ����
              (CPU_CHAR*	)"USART Queue",     //��Ϣ��������
              (OS_MSG_QTY	)QUEUE_SIZE,        //��Ϣ���г���
              (OS_ERR*	    )&err);             //������
    
    // Build KMP tables once
    buildKMPTables();
    
	OS_CRITICAL_ENTER();//�����ٽ���
	//������ʼ����
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//������ƿ�
				 (CPU_CHAR	* )"start task", 		//��������
                 (OS_TASK_PTR )start_task, 			//������
                 (void		* )0,					//���ݸ��������Ĳ���
                 (OS_PRIO	  )START_TASK_PRIO,     //�������ȼ�
                 (CPU_STK   * )&START_TASK_STK[0],	//�����ջ����ַ
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//�����ջ�����λ
                 (CPU_STK_SIZE)START_STK_SIZE,		//�����ջ��С
                 (OS_MSG_QTY  )0,					//�����ڲ���Ϣ�����ܹ����յ������Ϣ��Ŀ,Ϊ0ʱ��ֹ������Ϣ
                 (OS_TICK	  )0,					//��ʹ��ʱ��Ƭ��תʱ��ʱ��Ƭ���ȣ�Ϊ0ʱΪĬ�ϳ��ȣ�
                 (void   	* )0,					//�û�����Ĵ洢��
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //����ѡ��
                 (OS_ERR 	* )&err);				//��Ÿú�������ʱ�ķ���ֵ
	OS_CRITICAL_EXIT();	//�˳��ٽ���	 
	OSStart(&err);  //����UCOSIII
                 
    // Free allocated memory for KMP tables
    for (int i = 0; i < MAX_KEYWORDS; ++i) {
        free(kmpTables[i]);
    }
    
    return 0;
}

void start_task(void *p_arg) {
    OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//ͳ������                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//���ʹ���˲����жϹر�ʱ��
    CPU_IntDisMeasMaxCurReset();	
#endif

#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //��ʹ��ʱ��Ƭ��ת��ʱ��
	 //ʹ��ʱ��Ƭ��ת���ȹ���,ʱ��Ƭ����Ϊ1��ϵͳʱ�ӽ��ģ���1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
    
    OS_CRITICAL_ENTER();//�����ٽ���
    
    // Create keyword SPOR tasks
    OSTaskCreate((OS_TCB 	* )&SPORTaskTCB,		
				 (CPU_CHAR	* )"SPOR task", 		
                 (OS_TASK_PTR )SPOR_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )SPOR_PRIO,     
                 (CPU_STK   * )&SPOR_Stk[0],	
                 (CPU_STK_SIZE)SPOR_STK_SIZE/10,	
                 (CPU_STK_SIZE)SPOR_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);
                 
    // Create keyword detection tasks
    for (int i = 0; i < MAX_KEYWORDS; ++i) {
        OSTaskCreate((OS_TCB 	* )&TaskDetectTCB[i],       //������ƿ�
                     (CPU_CHAR	* )"Keyword Detect Task",   //��������
                     (OS_TASK_PTR )TaskDetect_task,         //������
                     (void       *)i,                       //���ݸ��������Ĳ���
                     (OS_PRIO	  )TASK_DETECT_PRIO,        //�������ȼ�
                     (CPU_STK   * )&TaskDetectStk[i][0],    //�����ջ����ַ
                     (CPU_STK_SIZE)TASK_DETECT_STK_SIZE / 10,   //�����ջ�����λ
                     (CPU_STK_SIZE)TASK_DETECT_STK_SIZE,        //�����ջ��С
                     (OS_MSG_QTY  )0,                       //�����ڲ���Ϣ�����ܹ����յ������Ϣ��Ŀ,Ϊ0ʱ��ֹ������Ϣ
                     (OS_TICK	  )0,                       //��ʹ��ʱ��Ƭ��תʱ��ʱ��Ƭ���ȣ�Ϊ0ʱΪĬ�ϳ���
                     (void   	* )NULL,                       //�û�����Ĵ洢��
                     (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //����ѡ��
                     (OS_ERR 	* )&err);                   //��Ÿú�������ʱ�ķ���ֵ
    } 
    
    OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//����ʼ����
    
    OS_CRITICAL_EXIT();	//�˳��ٽ���
	
}

// Function to build the KMP partial match table for a given pattern
void buildKMPTables(void) {
    for (int k = 0; k < MAX_KEYWORDS; ++k) {
        const char *pattern = TARGET_KEYWORDS[k];
        int len = 0;
        int i = 1;
        keywordLengths[k] = strlen(pattern);
        kmpTables[k] = (int *)malloc(keywordLengths[k] * sizeof(int));
        kmpTables[k][0] = 0;

        // Build the KMP table for the current pattern
        while (i < keywordLengths[k]) {
            if (pattern[i] == pattern[len]) {
                len++;
                kmpTables[k][i] = len;
                i++;
            } else {
                if (len != 0) {
                    len = kmpTables[k][len - 1];
                } else {
                    kmpTables[k][i] = 0;
                    i++;
                }
            }
        }
    }
}

// Function to perform KMP matching on the received keyword
void KMP(char *keyword, const char *pattern, int *kmpTable, int keywordLength) {
    int matchPosition = -1;
    for (int i = 0, j = 0; i < keywordLength; ) {
        if (keyword[i] == pattern[j]) {
            i++;
            j++;
            if (j == keywordLength) {
                matchPosition = i - j;
                break;
            }
        } else {
            if (j != 0) {
                j = kmpTable[j - 1];
            } else {
                i++;
            }
        }
    }

    // If a match is found, set the detection flag
    if (matchPosition != -1) {
        keywordDetected = 1;
    }
}

void TaskDetect_task(void *p_arg) {
    int keywordIndex = (int)(intptr_t)p_arg;  // Index of the keyword to be detected
    int keywordLength = keywordLengths[keywordIndex];
    char keyword[keywordLength + 1];  // Buffer to store the received keyword
    uint8_t index = 0;  // Index for the keyword buffer
    OS_ERR err;
    //OS_MSG_SIZE size;

    while (1) {
        // Pend (wait) for data from the USART queue
        char receivedData = (char)(intptr_t)OSQPend((OS_Q*		    )&usartQueue, 
                                                    (OS_TICK        )0, 
                                                    (OS_OPT		    )OS_OPT_PEND_NON_BLOCKING, 
                                                    (OS_MSG_SIZE*	)&size, 
                                                    (CPU_TS*		)0, 
                                                    (OS_ERR*		)&err);
        //printf("OS_MSG_SIZE: %d\n", size);
        if (err == OS_ERR_NONE) {
            // If the first character matches the beginning of the keyword
            if (index == 0 && (receivedData == TARGET_KEYWORDS[keywordIndex][0])) {
                keyword[0] = receivedData;
                index = 1;
            } else if(index > 0){
//                if (index > 0 && receivedData == TARGET_KEYWORDS[keywordIndex][index - 1] && keyword[index - 1] == ' ') {
//                    index = 0;
//                }
                // Store the received data in the keyword buffer
                keyword[index++] = receivedData;
                if (index == keywordLength) {
                    keyword[keywordLength] = '\0';
                    // Perform KMP matching
                    KMP(keyword, TARGET_KEYWORDS[keywordIndex], kmpTables[keywordIndex], keywordLength);
                    index = 0;  // Reset index after matching
                    if (keywordDetected) {
                        // Handle keyword detection (e.g., print a message or set a flag)
                        printf("Keyword detected: %s\n", TARGET_KEYWORDS[keywordIndex]);
                        //keywordDetected = 0;
                    }
                }
            }
        }
    }
    
}

void SPOR_task(void *p_arg){
    //OS_ERR err;
    uint32_t total = 60000;   //cycle	
#ifdef random_spor
    uint32_t seed = 0;
#endif
    
#ifdef progress_spor
    uint32_t time_ms = 10;
    uint32_t sum = 0;
#endif
    
    while (1)
    {
#ifdef random_spor
        srand(total);
        seed = rand() % 60 + 1;  //random number 1 - 60
#endif

#ifdef progress_spor        
        /* if(sum == 600){
            sum = 0;
            time_ms = 100;
        } */
		if(time_ms == 54900){
			time_ms = 0;
		}
#endif
		
#ifdef rw_spor
		delay_s(15);
        //OSTimeDlyHMSM(0,0,15,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ15s
#endif
		
#if defined(fix_spor) || defined(random_spor) || defined(progress_spor) 
		delay_ms(500 / 2);
        //OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_HMSM_STRICT,&err);
#endif

        GPIO_ResetBits(GPIOA, GPIO_Pin_4); //1 start
        printf("Power on\n");

        delay_s(1);
        //OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err);
        GPIO_ResetBits(GPIOA, GPIO_Pin_5); //2 start	
        printf("Press the power button\n");

        delay_s(5);                        //phone_power_on time
        //OSTimeDlyHMSM(0,0,5,0,OS_OPT_TIME_HMSM_STRICT,&err);
        GPIO_SetBits(GPIOA, GPIO_Pin_5);   //2 stop
        printf("Release the power button\n");
		 
/********** total_power_on time **********/
#ifdef rw_spor
		delay_s(60);
        //OSTimeDlyHMSM(0,0,60,0,OS_OPT_TIME_HMSM_STRICT,&err);
#endif

#ifdef random_spor
        delay_s(seed); 
        //OSTimeDlyHMSM(0,0,seed,0,OS_OPT_TIME_HMSM_STRICT,&err);
#endif

#ifdef progress_spor 
		delay_ms(time_ms / 2);
        //OSTimeDlyHMSM(0,0,0,time_ms,OS_OPT_TIME_HMSM_STRICT,&err);
#endif

#ifdef fix_spor
		delay_ms(500 / 2);
        //OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_HMSM_STRICT,&err);
#endif
/******************************************/

// detect action
#ifdef detect_word
        if(keywordDetected){
            break;
        }
#endif
        
        GPIO_SetBits(GPIOA, GPIO_Pin_4);   //1 stop
        printf("Power off\n");

#ifdef progress_spor 
		if(time_ms >= 10050 && time_ms < 30100){
			time_ms += 50;
		}
		else if(time_ms >= 30100){
			time_ms += 100;
		}
		else{
			time_ms += 10;
		}
        // sum++;
#endif

        total = total - 1;
        printf("residual cycles: %d\n", total);
        if(total == 0){
            break;
        }
    }
    
    printf("Func end\n");
}

void USART1_IRQHandler(void) {
    OS_ERR err;
#if SYSTEM_SUPPORT_OS 		//���SYSTEM_SUPPORT_OSΪ�棬����Ҫ֧��OS.
	OSIntEnter();    
#endif
    // Check if the USART receive interrupt flag is set
    if (USART1->SR & USART_SR_RXNE) {
        char receivedData = USART1->DR;
        // Post the received data to the queue
        OSQPost((OS_Q*		)&usartQueue, 
                (void*      )(intptr_t)receivedData, 
                (OS_MSG_SIZE)1, 
                (OS_OPT		)OS_OPT_POST_FIFO, 
                (OS_ERR*	)&err);
        if(err != OS_ERR_NONE)
		{
			printf("Error posting to queue: %d\n", err);
		}
    }
#if SYSTEM_SUPPORT_OS 	//���SYSTEM_SUPPORT_OSΪ�棬����Ҫ֧��OS.
	OSIntExit();  											 
#endif
}

