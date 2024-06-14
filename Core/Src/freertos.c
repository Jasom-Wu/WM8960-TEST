/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp_key.h"
#include "WM8960Apps.h"
#include "stdio.h"
#include "sdio.h"
#include "WM8960.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
uint32_t defaultTaskBuffer[ 1024 ];
osStaticThreadDef_t defaultTaskControlBlock;
osThreadId mainTaskHandle;
uint32_t MainTaskBuffer[ 1024 ];
osStaticThreadDef_t MainTaskControlBlock;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartMainTask(void const * argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadStaticDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 1024, defaultTaskBuffer, &defaultTaskControlBlock);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of mainTask */
  osThreadStaticDef(mainTask, StartMainTask, osPriorityNormal, 0, 1024, MainTaskBuffer, &MainTaskControlBlock);
  mainTaskHandle = osThreadCreate(osThread(mainTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  vTaskSuspend(mainTaskHandle);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  //HAL_GPIO_WritePin(USB_EN_GPIO_Port, USB_EN_Pin, GPIO_PIN_RESET);
  static portTickType PreviousWakeTime;
  const portTickType TimeIncrement = pdMS_TO_TICKS(50);
  PreviousWakeTime = xTaskGetTickCount();
  vTaskResume(mainTaskHandle);

  /* Infinite loop */
  for(;;)
  {
    KeyHandlerCore();
    vTaskDelayUntil(&PreviousWakeTime, TimeIncrement);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartMainTask */
/**
* @brief Function implementing the mainTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMainTask */
void StartMainTask(void const * argument)
{
  /* USER CODE BEGIN StartMainTask */
  static portTickType PreviousWakeTime;
  const portTickType TimeIncrement = pdMS_TO_TICKS(50);
  PreviousWakeTime = xTaskGetTickCount();
  ScanWavefiles("0:/WAV");
  /* Infinite loop */
  for(;;)
  {
    PlayWaveFile(Play_List[Music_Num]);
    RecordWaveFile("test_rec",16000);
    vTaskDelayUntil(&PreviousWakeTime, TimeIncrement);
  }
  /* USER CODE END StartMainTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void KeyProcessHandler(Key_Typedef keys){
  if(keys.k1 == CLICKED){
    if(audio_rec_state == AUDIO_RECORD || audio_rec_state == AUDIO_PAUSE){
      audio_rec_request = AUDIO_END;
      return;
    }
    if(audio_play_state == AUDIO_PLAY || audio_play_state == AUDIO_PAUSE || audio_play_state == AUDIO_END || audio_play_state == AUDIO_NONE){
      printf("Previous!\r\n");
      if(Music_Num==0)
        Music_Num = Music_Num_MAX-1;
      else
        Music_Num --;
      if(audio_play_state == AUDIO_PLAY || audio_play_state == AUDIO_PAUSE) {
        audio_play_request = AUDIO_CANCEL;
        while (audio_play_state != AUDIO_CANCEL);
      }
      audio_play_request = AUDIO_PLAY;
    }
  }else if (keys.k1 == LONG_PRESSED){
    if(audio_rec_state == AUDIO_RECORD)
      audio_rec_request = AUDIO_PAUSE;
    else if(audio_rec_state == AUDIO_NONE || audio_rec_state == AUDIO_END || audio_rec_state == AUDIO_CANCEL)
      audio_rec_request = AUDIO_RECORD;
    else if(audio_rec_state == AUDIO_PAUSE)
      audio_rec_request = AUDIO_RESUME;
  }
  if(keys.k2 == CLICKED){
    if(audio_rec_state == AUDIO_RECORD || audio_rec_state == AUDIO_PAUSE || audio_rec_state == AUDIO_END){
      audio_rec_request = AUDIO_CANCEL;
    }
    if(audio_play_state == AUDIO_PLAY || audio_play_state == AUDIO_PAUSE || audio_play_state == AUDIO_END || audio_play_state == AUDIO_NONE) {
      printf("Next!\r\n");
      if (Music_Num == Music_Num_MAX-1)
        Music_Num = 0;
      else
        Music_Num++;
      if(audio_play_state == AUDIO_PLAY || audio_play_state == AUDIO_PAUSE) {
        audio_play_request = AUDIO_CANCEL;
        while (audio_play_state != AUDIO_CANCEL);
      }
      audio_play_request = AUDIO_PLAY;
    }
  }else if(keys.k2 == LONG_PRESSED){
    if(audio_play_state == AUDIO_PLAY)
      audio_play_request = AUDIO_PAUSE;
    else if(audio_play_state == AUDIO_NONE || audio_play_state == AUDIO_END)
      audio_play_request = AUDIO_PLAY;
    else if(audio_play_state == AUDIO_PAUSE)
      audio_play_request = AUDIO_RESUME;
  }
}
/* USER CODE END Application */

