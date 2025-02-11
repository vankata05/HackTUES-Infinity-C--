/**
  ******************************************************************************
  *                                 GreenBo         
  *                           The Embedded Experts
  ******************************************************************************
  * @file    main.c
  * @brief
  *          
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 GreenBo.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by GreenBo under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  **/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "camera.h"
#include "lcd.h"
#include "BMP.h"
#include "servo.h"
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
DMA_HandleTypeDef hdma_usart1_tx;
uint8_t uart1_tx_busy_flag = 0;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// picture buffer
uint16_t pic[FrameWidth][FrameHeight];
uint8_t sendBuf[80 + (FrameWidth * FrameHeight * 2)];
uint32_t DCMI_FrameIsReady;
uint32_t Camera_FPS=0;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();
	
  /* Configure the MPU attributes for the QSPI 256MB without instruction access */
  MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
  MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress      = QSPI_BASE;
  MPU_InitStruct.Size             = MPU_REGION_SIZE_256MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);
	
  /* Configure the MPU attributes for the QSPI 8MB (QSPI Flash Size) to Cacheable WT */
  MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
  MPU_InitStruct.Number           = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress      = QSPI_BASE;
  MPU_InitStruct.Size             = MPU_REGION_SIZE_8MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO;
  MPU_InitStruct.IsBufferable     = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);
	
  /* Setup AXI SRAM in Cacheable WB */
  MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress      = D1_AXISRAM_BASE;
  MPU_InitStruct.Size             = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable     = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number           = MPU_REGION_NUMBER2;
  MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);
	
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
void LED_Blink(uint32_t Hdelay, uint32_t Ldelay)
{
  HAL_GPIO_WritePin(PE3_GPIO_Port, PE3_Pin, GPIO_PIN_SET);
  HAL_Delay(Hdelay - 1);
  HAL_GPIO_WritePin(PE3_GPIO_Port, PE3_Pin, GPIO_PIN_RESET);
  HAL_Delay(Ldelay - 1);
}
/* USER CODE END 0 */



/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)

{
  /* USER CODE BEGIN 1 */

#ifdef W25Qxx
  SCB->VTOR = QSPI_BASE;
#endif
  MPU_Config();
  CPU_CACHE_Enable();

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_I2C1_Init();
  MX_SPI4_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();

  BitMapFileHeader_init();
  /* USER CODE BEGIN 2 */
  uint8_t text[20];
	
  LCD_Test();

  sprintf((char *)&text, "Camera Not Found");
  LCD_ShowString(0, 58, ST7735Ctx.Width, 16, 16, text);

  //	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
  //	HAL_Delay(10);
  #ifdef TFT96
	Camera_Init_Device(&hi2c1, FRAMESIZE_QQVGA);
	#elif TFT18
	Camera_Init_Device(&hi2c1, FRAMESIZE_QQVGA2);
	#endif
	//clean Ypos 58
	ST7735_LCD_Driver.FillRect(&st7735_pObj, 0, 58, ST7735Ctx.Width, 16, BLACK);
	
  while (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)
  {

    sprintf((char *)&text, "Camera id:0x%x   ", hcamera.device_id);
    LCD_ShowString(0, 58, ST7735Ctx.Width, 16, 12, text);

    LED_Blink(5, 500);

    sprintf((char *)&text, "LongPress K1 to Run");
    LCD_ShowString(0, 58, ST7735Ctx.Width, 16, 12, text);

    LED_Blink(5, 500);
  }

  HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t)&pic, FrameWidth * FrameHeight * 2 / 4);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (DCMI_FrameIsReady)
    {
      DCMI_FrameIsReady = 0;
			
      #ifdef TFT96
      ST7735_FillRGBRect(&st7735_pObj, 0, 0, (uint8_t *)&pic[20][0], ST7735Ctx.Width, 80);
      #elif TFT18
      ST7735_FillRGBRect(&st7735_pObj,0,0,(uint8_t *)&pic[0][0], ST7735Ctx.Width, ST7735Ctx.Height);
      #endif
      sprintf((char *)&text,"%dFPS",Camera_FPS);
      LCD_ShowString(5,5,60,16,12,text);
      
      LED_Blink(1, 1);

      static uint8_t test_flag = 1;
      if(test_flag)
      {
        if (!uart1_tx_busy_flag)
        {
          test_flag = 0;
          uart1_tx_busy_flag = 1;
/**          uint16_t crc = Calculate_CRC((uint16_t *)&pic); // DEL uint16_t *
          pic[0][35] |= crc;
          pic[0][35] |= Calculate_CRC((uint16_t *)&pic);
          pic[1][0] = 0x55aa;
          pic[1][1] = 0x1122;**/
          Prepare_RGBdata();
          sendBuf[71] = Calculate_CRC();

          HAL_UART_AbortTransmit(&huart1);
          LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_0);
          //HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&sendBuf, sizeof(sendBuf));
          HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&sendBuf, 38480);
        }
      }
    }
    // HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI48, RCC_MCODIV_4);
}

/* USER CODE BEGIN 4 */

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	static uint32_t count = 0,tick = 0;
	
	if(HAL_GetTick() - tick >= 1000)
	{
		tick = HAL_GetTick();
		Camera_FPS = count;
		count = 0;
	}
	count ++;
	
  DCMI_FrameIsReady = 1;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while (1)
  {
    LED_Blink(5, 250);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  printf("Wrong parameters value: file %s on line %d\r\n", file, line);
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
