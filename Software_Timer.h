/*
 * Software_Timer.h
 *
 *  Created on: 16 Ağu 2026
 *      Author: Mansu
 */

#ifndef INC_SOFTWARE_TIMER_H_
#define INC_SOFTWARE_TIMER_H_


#include "main.h"
#include "stdbool.h"


typedef struct
{
	uint32_t 		startTime;
	uint32_t		intervalTime;

	bool			activated;

}STimer_t;

// 1- Time cevresel birimini sec ve baslat

void software_Timer_Init(TIM_HandleTypeDef *htim);

//2- Timer set et ve baslat

void Software_Timer_Set_Time(STimer_t *STimer, uint32_t intervalMs);

// 3- Zamanı al

uint32_t Software_Timer_Get_Time(void);

// 4- Gecen zamanı kontrol et

bool Software_Timer_Clock_Check_Elapsed_Time(STimer_t *STimer);

// 5- Zamanlayıcı durdur

void Software_Timer_Disable(STimer_t *STimer);




#endif /* INC_SOFTWARE_TIMER_H_ */
