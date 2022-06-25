/*
 * SystemInit.c
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#include <Cpu.h>
#include "CpuConfig.h"

void cpuInit(void)
{
	HAL_Init();
}

DriversExecStatus clockInit()
{
	if ( (PLL_M_VALUE == 0) || (PLL_N_VALUE == 0) || (PLL_P_VALUE == 0) || (PLL_Q_VALUE == 0)
			|| (APB1_PRESCALER == 0) || (APB2_PRESCALER == 0))
	{
		return (STATUS_ERROR);
	}

	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	 */
	 __HAL_RCC_SYSCFG_CLK_ENABLE();

	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */

#ifdef USE_RTC

#if defined (USE_RCC_HSE) && defined (USE_RTC_LSI)
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

#elif defined (USE_RCC_HSE) && defined (USE_RTC_LSE)
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

#elif defined (USE_RCC_HSI) && defined (USE_RTC_LSI)
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;

#elif defined (USE_RCC_HSI) && defined (USE_RTC_LSE)

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;

#elif defined (USE_RCC_MSI) && defined (USE_RTC_LSI)
	// not supported STM32F4
#elif defined (USE_RCC_MSI) && defined (USE_RTC_LSE)
	// not supported STM32F4
#endif /*USE_RCC_X && USE_RTC_X */

#else

#ifdef USE_RCC_HSE

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

#elif defined USE_RCC_HSI

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;

#endif /* USE_RCC_X */

#endif /* USE_RTC */

	RCC_OscInitStruct.PLL.PLLM = PLL_M_VALUE;
	RCC_OscInitStruct.PLL.PLLN = PLL_N_VALUE;
	RCC_OscInitStruct.PLL.PLLP = PLL_P_VALUE;
	RCC_OscInitStruct.PLL.PLLQ = PLL_Q_VALUE;

	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = APB1_PRESCALER;
	RCC_ClkInitStruct.APB2CLKDivider = APB2_PRESCALER;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		return (STATUS_ERROR);
	}


	return (STATUS_OK);
}
