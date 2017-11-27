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

/* FatFs includes */
#include "ff_gen_drv.h"
#include "sd_diskio.h"
	 
/* Definitions */
#define	NUM_MOTORS 16			//max number of motors
#define	TOUCH_DEADBAND 200	//msec delay so multiple touches aren't detected quickly
#define	BA_LOW_TIME 90	//msec delay to determine if a bill has been inserted
#define NUM_MOTOR_PULSES 200	//number of pulses to turn motors
#define NUM_ITEMS		16

/* Define the addresses of all admin mode states 
#define Blank 	&administrator[0] 
#define E1 		&administrator[1] 
#define E2		&administrator[2]
#define C		&administrator[3]
#define E3		&administrator[4]
*/

#define ADMIN_PASS				"EECE"	//must be A-E, 0-4
#define ADMIN_PASS_SIZE		4		//number of characters of admin password
#define FILENAME_SIZE 		16  //number of characters of filename
#define SELECTDISP_SIZE 	9		//number of characters of selection display
#define TEXT_SIZE					30	//number of characters of text buffer
#define SELECTION_SIZE		2		//number of characters of selection

/* Global Variables */
const char* adminPass = ADMIN_PASS;

/* State Machine Declaration for Admin Access */
typedef struct AdminState
{
	uint8_t access; //Access granted or not
	char currKey;
	adminState *nextAdminState;
}AdminState;

AdminState *admin[ADMIN_PASS_SIZE + 1]; //array of administrator states, one for each pass letter, last for admin granted
AdminState *currAdminState = admin[0];

uint8_t  status = 0;						// Push button status
volatile uint16_t x, y; 					// x,y location of touch input

char  *text;  		// Text buffer for display
char selectionPressed = NULL;// Input selection
//char* received_data;						// Characters recieved from UART transfer
char *selectDisp;  //display of id selection

volatile uint32_t tickCount;		// counter for 1ms timers
volatile uint32_t	lastTouch = 0;		// tick count of last touch detected on touchscreen
volatile uint32_t baTick = 0;				// tick count of last input detected from bill acceptor
volatile uint32_t lastActivityTick = 0;	//tick count of last activity by user (to keep track of when to sleep)

static TS_StateTypeDef  TS_State;
typedef enum State{SLEEP,SLIDE_CARD,SELECTION,STOCK,DONE} UIState;	//UI states

UIState state = SLIDE_CARD;   // UI state
UIState prevState = SLEEP;   // Previous UI state

volatile uint32_t oneDTick, fiveDTick, tenDTick, twentyDTick;	//tick count of last $1,$5,$10,$20 input

typedef enum Button_Area
{
	SELECT,
	PASS
}buttonArea = SELECT;

TIM_HandleTypeDef TimHandle;
uint32_t uwPrescalerValue;

uint8_t motorId = 0;
char *fileName; // Name of file to read or write

/* Flags */
uint8_t updateBalance = 1;	    // Refresh balance display
uint8_t updateCost = 1;					// Refresh cost display flag
uint8_t updateSelection = 1;	  // Refresh selection display flag

/*
typedef struct state states_type;

states_type administrator[9] = {
{0,{Blank,Blank,Blank,Blank,E1}},
{0,{Blank,Blank,Blank,Blank,E2}},
{0,{Blank,Blank,C,Blank,Blank}},
{0,{Blank,Blank,Blank,Blank,E3}},
{1,{Blank,Blank,Blank,Blank,Blank}}};

states_type *current_state = administrator;
*/

//Structure for stocked items
typedef struct Item
{
	uint8_t itemID;	//Same as motor ID.
	char *selectionNum; //selection number for file name
	float itemCost;	//Self explanatory. Seriously, why are you reading this?
	uint8_t itemCount; //number in stock
}Item;

Item *item;

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

User *usr;

/* Exported functions */
uint8_t CheckForUserInput(void);
static uint8_t getItemId(uint8_t selection[]);
static void turnMotor(uint8_t motorId);
static void motorPinsInit(void);
void billAcceptorInit(void);
static void TIM7_Init(void);
static void Error_Handler(void);
//float getCost(uint8_t letter,uint8_t number);
void adminStateInit(void);
void textInit(void);
uint8_t validateSelection(char selectionPressed, uint8_t selectionIndx);
uint8_t checkButton(uint16_t xCoord, uint16_t yCoord, uint8_t screen);
uint8_t getButtonDef(uint8_t row,uint8_t column, uint8_t screen);
void drawKeypad(uint8_t screen);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */


