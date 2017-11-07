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
	 
/* Definitions */
#define PASSLEN		5
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

#define ADMIN_PASS				{'E','E','C','E'}
#define ADMIN_PASS_SIZE		4

/* Global Variables */
char[ADMIN_PASS_SIZE] adminPass = ADMIN_PASS;
uint8_t  status = 0;						// Push button status
volatile uint16_t x, y; 					// x,y location of touch input
float cost = 0.00; 							// Selected item cost
//float balance = 0.00;						// Current balance

uint8_t  text[30] = {0,0};  		// Text buffer for display
uint8_t selectionPressed = NULL;// Input selection
//char* received_data;						// Characters recieved from UART transfer
uint8_t selectDisp[9] = {(uint8_t)'_',(uint8_t)'_',(uint8_t)'_',(uint8_t)'_',
(uint8_t)'_',(uint8_t)'_',(uint8_t)'_',(uint8_t)'_',(uint8_t)'_'};  //display of id selection

//char* passkey; //password selection
//uint8_t passkeyIdx = 0;	//current index of admin passcode
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
unsigned int ind = 0;						// Index into state machine	
int adminCheck = 0;							// Yes or No to entering as an admin
char *amount = "";					  // Money to credit to the account
char *fileName;								  // Name of file to read or write
//uint8_t *amountOnCard;					// Money on Card


//char* password = "12345";
int j = 0;

/* Flags */
uint8_t updateBalance = 1;	    // Refresh balance display
uint8_t updateCost = 1;					// Refresh cost display flag
uint8_t updateSelection = 1;	  // Refresh selection display flag
uint8_t updatePw = 1;						// Refresh password display flag
uint8_t clear = 0;	            // Clear selection Flag
uint8_t admin = 0;							// Admin Flag

	
/* State Machine Declaration for Admin Access */
typedef struct AdminState
{
	uint8_t access; //Access granted or not
	uint8_t currKey;
	adminState nextAdminState;
}AdminState;

AdminState admin[ADMIN_PASS_SIZE + 1]; //array of administrator states, one for each pass letter, last for admin granted

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
void adminStateInit();

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */


