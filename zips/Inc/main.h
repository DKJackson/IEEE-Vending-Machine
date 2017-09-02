/* Define to prevent recursive inclusion */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes */
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include <stdlib.h>
#include "fatfs_storage.h"
#include "display.h"
#include "configurations.h"

/* FatFs includes component */
#include "ff_gen_drv.h"
#include "sd_diskio.h"
/* Exported functions */
uint8_t CheckForUserInput(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */


