/*
 * Software_Timer.c
 *
 *  Created on: 16 Ağu 2026
 *      Author: Mansu
 */


#include "Software_Timer.h"

uint32_t msTick = 0;
// 1- Time cevresel birimini sec ve baslat

void software_Timer_Init(TIM_HandleTypeDef *htim)
{
	HAL_TIM_Base_Start_IT(htim);
}

//2- Timer set et ve baslat

void Software_Timer_Set_Time(STimer_t *STimer, uint32_t intervalMs) // intervalMs kullanıcın istediği zaman
{
	STimer->startTime 			= 	Software_Timer_Get_Time();
	STimer->intervalTime 		= 	intervalMs; // Kullanıcın calısmasını istdiği zaman
	STimer->activated			=	true;
}

// 3- Zamanı al

uint32_t Software_Timer_Get_Time(void)
{
	return msTick;
}

// 4- Gecen zamanı kontrol et

bool Software_Timer_Clock_Check_Elapsed_Time(STimer_t *STimer) // ne kadar süre geçtiğini ve bitip bitmediğini kontrol eden fonksiyon
{
 	if(STimer->activated == true)
	{
		uint32_t currentTick = Software_Timer_Get_Time();

		if(STimer->startTime <= currentTick)
		{
			if(currentTick - STimer->startTime >= STimer->intervalTime)
			{
				Software_Timer_Disable(STimer);

				return true;
			}

		}
		else
		{
			if((0xFFFFFFFF - (STimer->startTime - currentTick)) >= STimer->intervalTime)
			{
				Software_Timer_Disable(STimer);

				return true;
			}
		}
	}
	return false;
}

// 5- Zamanlayıcı durdur

void Software_Timer_Disable(STimer_t *STimer)
{
	STimer->activated		=	false;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM11)
	{
		//1Ms kesme calisti demek
		msTick++;
	}
}






