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
#define PASSLEN		5
#define	NUM_MOTORS 16			//max number of motors
#define	TOUCH_DEADBAND 200	//msec delay so multiple touches aren't detected quickly
#define	BA_LOW_TIME 90	//msec delay to determine if a bill has been inserted
#define NUM_MOTOR_PULSES 200	//number of pulses to turn motors
#define NUM_ITEMS		16



//Structure for stocked items
struct Item
{
	uint8_t itemID;	//Same as motor ID.  Also, same as index for item array.  Why is this here?  Because.
	uint8_t itemCost;	//Self explanatory. Seriously, why are you reading this?
	uint8_t itemCount; //number in stock
};

struct Item item[NUM_ITEMS];

//user type is regular user or administrator
typedef enum User_Type
{
	USER,
	ADMIN
}U_Type;

	
//Structure for user accounts
typedef struct User
{
	uint8_t studentID;
	uint8_t wildcatNum;
	float balance;
	U_Type userType;
}User;

/* Exported functions */
uint8_t CheckForUserInput(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */


