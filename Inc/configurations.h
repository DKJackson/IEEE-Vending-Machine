/* Define to prevent recursive inclusion */
#ifndef __CONFIGURATIONS_H
#define __CONFIGURATIONS_H

/* Includes */
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery.h"
#include <stdlib.h>

/* Exported functions */	 
void SystemClock_Config(void);
void MPU_Config(void);
void CPU_CACHE_Enable(void);

#endif 
