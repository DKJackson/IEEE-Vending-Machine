/* Includes */
#include "main.h"
#include "uart.h"
#include "display.h"
#include "configurations.h"
#include "database.h"
#include <math.h>

/*********************** FAT FS Stuff ************************
FATFS SD_FatFs;  // File system object for SD card logical drive 
char SD_Path[4]; // SD card logical drive path 
char* pDirectoryFiles[MAX_BMP_FILES];
uint8_t  ubNumberOfFiles = 0;
uint32_t uwBmplen = 0;
**************************************************************/

/* Define the addresses of all my states */
#define Blank 	&administrator[0] 
#define E1 		&administrator[1] 
#define E2		&administrator[2]
#define C		&administrator[3]
#define E3		&administrator[4]

static TS_StateTypeDef  TS_State;
enum STATE{SLEEP,SLIDE_CARD,SELECTION,STOCK,DONE};

/* Global Variables */
uint8_t  status = 0;						// Push button status
volatile uint16_t x, y; 									// x,y location of touch input
float cost = 0.00; 							// Selected item cost
//float balance = 0.00;						// Current balance
volatile enum STATE state = SLIDE_CARD;   // UI state
enum STATE prevState = SLEEP;   // Previous UI state
uint8_t  text[30] = {0,0};  		// Text buffer for display
uint8_t selectionPressed = NULL;// Input selection
char* received_data;						// Characters recieved from UART transfer
uint8_t selectDisp[9] = {(uint8_t)'_',(uint8_t)'_',(uint8_t)'_',(uint8_t)'_',
(uint8_t)'_',(uint8_t)'_',(uint8_t)'_',(uint8_t)'_',(uint8_t)'_'};  //display of selection
char* passkey; //password selection
uint8_t passkeyIdx = 0;	//current index of admin passcode
volatile uint32_t tickCount;		// counter for 1ms timers
volatile uint32_t	lastTouch = 0;		// tick count of last touch detected on touchscreen
volatile uint32_t baTick = 0;				// tick count of last input detected from bill acceptor
volatile uint32_t lastActivityTick = 0;	//tick count of last activity by user (to keep track of when to sleep)

volatile uint32_t oneDTick, fiveDTick, tenDTick, twentyDTick;	//tick count of last $1,$5,$10,$20 input

//Structure for stocked items
struct Item
{
	uint8_t itemID;	//Same as motor ID.  Also, same as index for item array.  Why is this here?  Because.
	uint8_t itemCost;	//Self explanatory. Seriously, why are you reading this?
	uint8_t itemCount; //number in stock
};

struct Item item[NUM_ITEMS];

typedef enum User_Type
{
	USER,
	ADMIN
}U_Type;

	
//Structure for user accounts
struct User
{
	uint8_t studentID;
	uint8_t wildcatNum;
	float balance;
	U_Type userType;
};

struct User usr;

enum Button_Area
{
	SELECT,
	PASS
}buttonArea = SELECT;

TIM_HandleTypeDef TimHandle;
uint32_t uwPrescalerValue;

static void motorPinsInit(void);
uint8_t motorId = 0;
static uint8_t getItemId(uint8_t selection[]);
static void turnMotor(uint8_t motorId);
unsigned int ind = 0;						// Index into state machine	
int adminCheck = 0;							// Yes or No to entering as an admin
char *amount = "15";					  // Money to credit to the account
char *fileName;								  // Name of file to read or write
uint8_t *amountOnCard;					// Money on Card


char* password = "12345";
int j = 0;

/*States for password characters.  Password must be entered in correct order
enum User_State
{
	USER,
	ADMIN
} userState = USER;
*/

/* Flags */
uint8_t updateBalance = 1;	    // Refresh balance display
uint8_t updateCost = 1;					// Refresh cost display flag
uint8_t updateSelection = 1;	  // Refresh selection display flag
uint8_t updatePw = 1;						// Refresh password display flag
uint8_t clear = 0;	            // Clear selection Flag
uint8_t admin = 0;							// Admin Flag

/* State Machine Declaration for Admin Access */
struct state
{
	unsigned int access; //Access granted or not
	struct state *next[5];
};

typedef struct state states_type;

states_type administrator[9] = {
{0,{Blank,Blank,Blank,Blank,E1}},
{0,{Blank,Blank,Blank,Blank,E2}},
{0,{Blank,Blank,C,Blank,Blank}},
{0,{Blank,Blank,Blank,Blank,E3}},
{1,{Blank,Blank,Blank,Blank,Blank}}};

states_type *current_state = administrator;

void billAcceptorInit(void);
static void TIM7_Init(void);
static void Error_Handler(void);
	
int main(void)
{
	/***************************************************************
	*	Initialize stock items
	*	This needs to be updated with a read from the SD card
	*	(and moved after the sd init)
	*
	*
	***************************************************************/
	for(uint8_t i = 0; i < NUM_ITEMS; i++)
	{
		item[i].itemID = i;
		item[i].itemCost = 0;
		item[i].itemCount = 0;
	}
	
	/* Configure the MPU attributes as Write Through */
//  MPU_Config();

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32F7xx HAL library initialization */
  HAL_Init();
	
  /* Configure the system clock to 200 MHz */
  SystemClock_Config();
  
  /* Configure LED1 */
  BSP_LED_Init(LED1);
  
  /* Configure LCD */
	LCD_Configuration(); 
	
	/* Initialize Pins for Bill Acceptor */
	billAcceptorInit();
	
	/* Initialize pins for motor control */
	motorPinsInit();
	
	//Initiialize Timer 
	TIM7_Init();
	
	/* Clear the LCD */
  clearScreen();
	
  /* Initialize Touchscreen */	
	status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	
	/* Configure TS interrupt */
	BSP_TS_ITConfig();
	
	/* Initialize the Fat File System Database on the SD card */
	databaseInit();
	/* Ensure Touchscreen initialized properly */
  if (status != TS_OK)
  {
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 95, (uint8_t *)"ERROR", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 80, (uint8_t *)"Touchscreen cannot be initialized", CENTER_MODE);	
		while(1)
		{
		}
  }	
	
	while(1)
	{
		// Waiting for user to swipe/enter wildcat card
		while(state == SLIDE_CARD)
		{
			if(prevState != SLIDE_CARD)
			{
				prevState = SLIDE_CARD;
				drawCardFrame();			
				BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
			}						
			/***********************************UPDATE SELECTION***********************************/			
			if(x != 0 && y != 0)
			{					
				// Get the Button that was pressed
				selectionPressed = checkButton(x,y,2);
				HAL_Delay(150);
				// Delete last character of selection
				if(selectionPressed == (uint8_t) '<')
				{
					for(int i=8; i >= 0; i--)
					{
						if(selectDisp[i] != (uint8_t) '_')
						{
							selectDisp[i] = (uint8_t) '_';
							updateSelection = 1;
							break;
						}
					}
					clear = 1;
				}
				// Delete entire selection
				else if(selectionPressed == (uint8_t) 'X')
				{
					for(int i=0; i <= 8; i++)
					{
						selectDisp[i] = (uint8_t) '_';
					}
					clear = 1;
				}		
				// Update ID Number on Screen
				for(int i=0; i <= 8; i++)
				{
					if(clear == 1 || selectionPressed == (uint8_t) '>')
						break;
					if(selectDisp[i] == (uint8_t)'_')
					{
						selectDisp[i] = (selectionPressed == NULL) ? (uint8_t)'_' : selectionPressed;
						updateSelection = 1;
						break;
					}			
				}
				// Clear touch coordinates and button pressed flag
				x = 0; y = 0; clear = 0;
				// Go to item selection
				if(selectDisp[8] != (uint8_t) '_' && selectionPressed == (uint8_t) '>')
				{
					state = SELECTION;
					
					/********************************* Get user data and update usr struct************************************/
					
					
					
					
					
					
					
					
					
					
					
					
					
					
					// Setup the name of the file which can only be 8 characters + ".TXT"
					fileName = (char*)&selectDisp+1;
					strncat (fileName, ".TXT", 4);
					writeFile((uint8_t*)amount, (char*) fileName);
					amountOnCard = readFile((char*) fileName);
					// File Does Not Exist
					if(amountOnCard == (uint8_t*)'X')
					{
						usr.balance += 0;
					}
					
					// Find out how much money is on the account
					else
					{
						for(int i=3 ; i >= 0; i--)
						{
							if(amountOnCard[i] != NULL)
							{
								usr.balance += (pow(10,j) * (amountOnCard[i] - 48));
								j +=1;
							}
						}
					}
					updateBalance = 1;
					// Clear Display Values
					for(int i=0; i <= 8; i++)
					{
						selectDisp[i] = (uint8_t) '_';
					}
				}
				selectionPressed = NULL;						
			}
			/***********************************************************************************/
			if(updateSelection == 1)
			{
				sprintf((char*)text, "%c%c%c%c%c%c%c%c%c",selectDisp[0],selectDisp[1],selectDisp[2],selectDisp[3],
				selectDisp[4],selectDisp[5],selectDisp[6],selectDisp[7],selectDisp[8]);
				BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
				BSP_LCD_DisplayStringAt(5, 120, (uint8_t*) text, LEFT_MODE);
			}
		}
		
		
		// When touch occurs
		while(state == SELECTION)
		{
			// Draw the UI if state has changed
			if(prevState != SELECTION)
			{
				prevState = SELECTION; // Ensure we dont redraw the UI unless necesarry				
				drawDisplayFrame();		 // Render display frame
				drawKeypad();					 // Render selection buttons
				
				//Render Balance area
				BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
				BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
				BSP_LCD_DisplayStringAt(10, 150, (uint8_t*) "Balance", LEFT_MODE);
				BSP_LCD_DisplayStringAt(A_BUTTON_XPOS, A_BUTTON_YPOS, (uint8_t*) "*", LEFT_MODE);
					
				BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
									
			}
			
			//displayCost(selectDisp[0],selectDisp[1]);
			
			// Get the cost of the selected item
			cost = getCost(selectDisp[0],selectDisp[1]);
			
			// Display cost in red or green based on balance and the price of the item
			BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
			if(usr.balance < cost)
				BSP_LCD_SetTextColor(LCD_COLOR_RED);
			else
				BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			
			// Display cost of selected item
			sprintf((char*)text, "$%0.2f",cost);
			BSP_LCD_DisplayStringAt(10, 115, (uint8_t*) text, LEFT_MODE);
			BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
			BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
			BSP_LCD_SetFont(&Font24);	
			
//			if(updateKeypad == 1)
//			{	
//			}
			
			
			// If button press detected
			if(x != 0 && y != 0)
			{
/*				if(x >= A_BUTTON_XPOS && x <= A_BUTTON_XPOS + 20 && y >= A_BUTTON_YPOS - 24 && y <= A_BUTTON_YPOS)
				{
					x = 0;
					y = 0;
					state = STOCK;
					continue;
				}
*/				

				// Get the Button that was pressed
				selectionPressed = checkButton(x,y,1);
				
				switch(selectionPressed)
				{
					case ((uint8_t)'A'):
						ind = 0;
					break;
					case ((uint8_t)'B'):
						ind = 1;
					break;
					case ((uint8_t)'C'):
						ind = 2;
					break;
					case ((uint8_t)'D'):
						ind = 3;
					break;
					case ((uint8_t)'E'):
						ind = 4;
					break;
					default:
						ind = 0;
					break;
				}
				
				HAL_Delay(150);
				
				current_state = current_state->next[ind];
				
				if(current_state->access == 1)
					state = STOCK;
				// Delete last character of selection
				if(selectionPressed == (uint8_t) '<')
				{
					if(selectDisp[1] != (uint8_t) '_')
					{
						selectDisp[1] = (uint8_t) '_';
						updateSelection = 1;
					}

					else if(selectDisp[0] != (uint8_t) '_')
					{
						selectDisp[0] = (uint8_t) '_';
						updateSelection = 1;
					}

					clear = 1;

					HAL_Delay(100);
				}

				// Delete entire selection
				else if(selectionPressed == (uint8_t) 'X')
				{
					selectDisp[1] = (uint8_t) '_';
					selectDisp[0] = (uint8_t) '_';

					clear = 1;


				}		
				// Vend Item if available and sufficient funds
				else if(selectionPressed == (uint8_t) '>')
				{
					// Valid Selection
					if(selectDisp[0] != (uint8_t) '_' && selectDisp[1] != (uint8_t) '_')
					{
						if(usr.balance >= cost)
						{
							// Vend item
							usr.balance -= cost;
							//amount = (char*)balance;
							writeFile((uint8_t*)&(usr.balance), (char*) fileName);
							state = SLIDE_CARD;
						}
					}
					selectDisp[1] = (uint8_t) '_';
					selectDisp[0] = (uint8_t) '_';
					clear = 1;
				}				
				// Validate that selection is valid for the selection place
				// ie selectDisp[0] must be A-E, < ,X, or >, selectDisp[1] must be 0-4, < ,X , or >
				if(selectDisp[0] == (uint8_t) '_')
				{
					if(selectionPressed != (uint8_t) 'A' && selectionPressed != (uint8_t) 'B' && selectionPressed != (uint8_t) 'C' 
					&& selectionPressed != (uint8_t) 'D' && selectionPressed != (uint8_t) 'E')
					{
						selectionPressed = NULL;
					}
				}

				else if(selectDisp[1] == (uint8_t) '_')
				{
					if(selectionPressed != (uint8_t) '0' && selectionPressed != (uint8_t) '1' && selectionPressed != (uint8_t) '2' 


					&& selectionPressed != (uint8_t) '3' && selectionPressed != (uint8_t) '4')
					{
						selectionPressed = NULL;
					}
				}					
				// Assign 1st selection if valid button was pressed
				if(selectDisp[0] == (uint8_t)'_' && clear != 1)
				{
					selectDisp[0] = (selectionPressed == NULL) ? (uint8_t)'_' : selectionPressed;
					updateSelection = 1;


				}				
				// Assign 2nd selection if valid button was pressed
				else if(selectDisp[1] == (uint8_t)'_' && clear != 1)
				{
					selectDisp[1] = (selectionPressed == NULL) ? (uint8_t)'_' : selectionPressed;
					updateSelection = 1;						

				}					
				// Clear touch coordinates and button pressed flag
				x = 0; y = 0; clear = 0;						


				selectionPressed = NULL;
				
				// Delete last character of selection
				if(selectionPressed == (uint8_t) '<')
				{
					if(selectDisp[1] != (uint8_t) '_')
					{
						selectDisp[1] = (uint8_t) '_';
					}
					
					else if(selectDisp[0] != (uint8_t) '_')
					{
						selectDisp[0] = (uint8_t) '_';
					}
					
					clear = 1;
					updateSelection = 1;
					HAL_Delay(100);
				}
				
				// Delete entire selection
				else if(selectionPressed == (uint8_t) 'X')
				{
					selectDisp[1] = (uint8_t) '_';
					selectDisp[0] = (uint8_t) '_';
					updateSelection = 1;
					clear = 1;
				}
				
				// Validate that selection is valid for the selection place
				// ie selectDisp[0] must be A-E selectDisp[1] must be 0-4
				if(selectDisp[0] == (uint8_t) '_')
				{
					selectionPressed = validateSelection(selectionPressed,0) ? selectionPressed : NULL;
				}
				
				else if(selectDisp[1] == (uint8_t) '_')
				{
					selectionPressed = validateSelection(selectionPressed,1) ? selectionPressed : NULL;
				}
				
				// Assign 1st selection if valid button was pressed
				if(selectDisp[0] == (uint8_t)'_' && clear != 1)
				{
					selectDisp[0] = (selectionPressed == NULL) ? (uint8_t)'_' : selectionPressed;
					updateSelection = 1;
				}
				
				// Assign 2nd selection if valid button was pressed
				else if(selectDisp[1] == (uint8_t)'_' && clear != 1)
				{
					selectDisp[1] = (selectionPressed == NULL) ? (uint8_t)'_' : selectionPressed;
					updateSelection = 1;						
				}
				
				//confirmation button pressed
				if(selectionPressed == (uint8_t)'>')
				{
					if(usr.balance >= cost)
					{
						//get id of motor to turn
						motorId = getItemId(selectDisp);
						
						//selection has been made, so turn motors if balance is greater than or equal to cost
						if(motorId != NULL)
						{
							//Decrement item count
							item[motorId].itemCount--;
							
							usr.balance -= cost;
							
						
							writeFile((uint8_t*)&usr, (char*) fileName);
							
							state = SLIDE_CARD;
								
							//turn motors
							turnMotor(motorId);
						}
					}
				}
				
				// Clear touch coordinates, selection clear flag and button pressed flag
				x = 0;
				y = 0;
				clear = 0;
				selectionPressed = NULL;						
			}
		
			// When valid button pressed display selections
			if(updateSelection == 1)
			{
				BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
				BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
				sprintf((char*)text, "%c%c",selectDisp[0],selectDisp[1]);
				BSP_LCD_DisplayStringAt(10, 58, (uint8_t*) text, LEFT_MODE);
				updateSelection = 0;
			}
			
			// When money inserted display updated balance			
			if(updateBalance == 1)
			{
				BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
				BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
				sprintf((char*)text, "$%0.2f",usr.balance);
				BSP_LCD_DisplayStringAt(10, 172, (uint8_t*) text, LEFT_MODE);
				
				/**************************write usr to file**********************************/
				
				
				
				
				
				
				
				
				updateBalance = 0;
			}
			
		}
		
		// Shut down UI and	Enable Low Power Mode	::UNUSED
		while(state == SLEEP)
		{
			if(prevState != SLEEP)
			{
				prevState = SLEEP;
				// Clear LCD
				BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
				clearScreen();
					
				//enable low power mode
				//HAL_PWR_EnterSLEEPMode();
			}
		}
		
		while(state == STOCK)
		{
			
			
			if(usr.userType != ADMIN)
			{
				state = SELECTION;
			}
			
			
			else
			{
				// Draw the UI if state has changed
				if(prevState != STOCK)
				{
					prevState = STOCK; // Ensure we dont redraw the UI unless necesarry
					
					drawAdminDispFrame();		 // Render display area
				
					//drawPassKeypad();					 // Render passkey buttons			
					BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
					selectDisp[0] = (uint8_t)'_';
					selectDisp[1] = (uint8_t)'_';
					updateSelection = 1;
					selectionPressed = NULL;
				}
				
				
				if(x != 0 && y != 0)
				{
		
						//If not a passkey button, check for selection buttons
						if(selectionPressed == NULL)
						{
							selectionPressed = checkButton(x,y,2);
							buttonArea = SELECT;
						}
						
						//User has pressed a password key
						else
						{
							buttonArea = PASS;
						}
						
						if(selectionPressed != NULL)
						{
							if(buttonArea == SELECT)
							{
								// ***********************************		COPIED FROM ABOVE, NEEDS OPTIMIZATION
								if(selectionPressed == (uint8_t) '<')
								{
									if(selectDisp[1] != (uint8_t) '_')
									{
										selectDisp[1] = (uint8_t) '_';
									}
									
									else if(selectDisp[0] != (uint8_t) '_')
									{
										selectDisp[0] = (uint8_t) '_';
									}
									
									clear = 1;
									updateSelection = 1;
									HAL_Delay(100);
								}
								
								// Delete entire selection
								else if(selectionPressed == (uint8_t) 'X')
								{
									selectDisp[1] = (uint8_t) '_';
									selectDisp[0] = (uint8_t) '_';
									updateSelection = 1;
									clear = 1;
								}
								
								// Validate that selection is valid for the selection place
								// ie selectDisp[0] must be A-E selectDisp[1] must be 0-4
								if(selectDisp[0] == (uint8_t) '_')
								{
									selectionPressed = validateSelection(selectionPressed,0) ? selectionPressed : NULL;
								}
								
								else if(selectDisp[1] == (uint8_t) '_')
								{
									selectionPressed = validateSelection(selectionPressed,1) ? selectionPressed : NULL;
								}
								
								// Assign 1st selection if valid button was pressed
								if(selectDisp[0] == (uint8_t)'_' && clear != 1)
								{
									selectDisp[0] = (selectionPressed == NULL) ? (uint8_t)'_' : selectionPressed;
									updateSelection = 1;
								}
								
								// Assign 2nd selection if valid button was pressed
								else if(selectDisp[1] == (uint8_t)'_' && clear != 1)
								{
									selectDisp[1] = (selectionPressed == NULL) ? (uint8_t)'_' : selectionPressed;
									updateSelection = 1;						
								}
								
								//confirmation button pressed so update the item cost
								if(selectionPressed == (uint8_t)'>' && selectDisp[0] != NULL && selectDisp[1] != NULL)
								{
									
									
									//Change selection color to green to confirm the cost has been updated
									BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
									updateSelection = 1;
								}
								
								
								// When valid button pressed display selections
								if(updateSelection == 1)
								{
									sprintf((char*)text, "%c%c",selectDisp[0],selectDisp[1]);
									BSP_LCD_DisplayStringAt(10, 58, (uint8_t*) text, LEFT_MODE);
									updateSelection = 0;
								}
								
							}
							// *************************************************** END COPIED AREA
							
							
							else
							{
								
							}
						}
					
					
					selectionPressed = NULL;
				}
				
				// When valid button pressed display selections
				if(updateSelection == 1)
				{
					BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
					BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
					sprintf((char*)text, "%c%c",selectDisp[0],selectDisp[1]);
					BSP_LCD_DisplayStringAt(10, 58, (uint8_t*) text, LEFT_MODE);
					updateSelection = 0;
				}
				
				// When money inserted display updated balance			
				if(updateCost == 1)
				{
					BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
					BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
					sprintf((char*)text, "$%0.2f",cost);
					BSP_LCD_DisplayStringAt(10, 115, (uint8_t*) text, LEFT_MODE);
					updateCost = 0;
				}
				
			}
		}
	}
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
* @brief  billAcceptorInit
*					Initializes the bill acceptor on pins PB9, PB8, PC7 and PC6
*					
* @param  None
* @retval None
*/
void billAcceptorInit(void)
{	
	// Use pins on GPIO Port B 
	GPIO_TypeDef*     gpio_port;
  gpio_port = GPIOB;
 
	// Enable the GPIO Port B Clock 
	__HAL_RCC_GPIOB_CLK_ENABLE();
	// Enable the GPIO Port C Clock 
	__HAL_RCC_GPIOC_CLK_ENABLE();
 	
	// GPIO Configuration Settings
  GPIO_InitTypeDef gpio_init_structure;
  gpio_init_structure.Pull = GPIO_NOPULL;			       // No pull up or pull down resistors
  gpio_init_structure.Speed = GPIO_SPEED_FAST;	
  gpio_init_structure.Mode = GPIO_MODE_IT_RISING_FALLING;   // Triggered at rising and falling edge
	
	// Initialize Pin PB9
	gpio_init_structure.Pin = GPIO_PIN_9; 
  HAL_GPIO_Init(gpio_port, &gpio_init_structure);	
	// Initialize Pin PB8
  gpio_init_structure.Pin = GPIO_PIN_8; 			     
  HAL_GPIO_Init(gpio_port, &gpio_init_structure);	
		
	// Use pins on GPIO Port C 
  gpio_port = GPIOC; 
 	
	// Initialize Pin PC7 
  gpio_init_structure.Pin = GPIO_PIN_7;
  HAL_GPIO_Init(gpio_port, &gpio_init_structure);			
	// Initialize Pin PC6
  gpio_init_structure.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(gpio_port, &gpio_init_structure);
	
	// Set ISR to lowest priority
	HAL_NVIC_SetPriority((IRQn_Type)(EXTI9_5_IRQn), 0x000F, 0x0000);
	HAL_NVIC_EnableIRQ((IRQn_Type)EXTI9_5_IRQn);
}

/**
* @brief  EXTI9_5_IRQHandler
*					Handler for Bill Acceptor Lines
*					
* @param  None
* @retval None
*/
void EXTI9_5_IRQHandler(void)
{
	// $1 Inserted
	if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_9) != RESET)
	{
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);
		
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) == GPIO_PIN_RESET)
		{
			oneDTick = tickCount;
		}
		
		else
		{
			//what about tick rollover since last check? (unlikely, but possible)
			//if(tickCount < oneDTick)		do stuff with this
			
			if(tickCount - oneDTick > BA_LOW_TIME)
			{
				updateBalance = 1;
				usr.balance += 1;
			}
		}
		
		
		// Wait 90 ms and make sure pin is still low (low for 100 ms if bill inserted)
		HAL_Delay(90);
		
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) == GPIO_PIN_RESET)
		{
			// Wait for 20 more ms and make sure pin went back high
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) == GPIO_PIN_SET)
			{
				updateBalance = 1;
				usr.balance += 1;
			}
		}
		
		
	}
	// $5 Inserted
	else if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_8) != RESET)
	{
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_8);
		
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8) == GPIO_PIN_RESET)
		{
			fiveDTick = tickCount;
		}
		
		else
		{
			//what about tick rollover since last check? (unlikely, but possible)
			//if(tickCount < fiveDTick)		do stuff with this
			
			if(tickCount - fiveDTick > BA_LOW_TIME)
			{
				updateBalance = 1;
				usr.balance += 5;
			}
		}
		
		
		// Wait 90 ms and make sure pin is still low (low for 100 ms if bill inserted)
		HAL_Delay(90);
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8) == GPIO_PIN_RESET)
		{
			// Wait for 20 more ms and make sure pin went back high
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8) == GPIO_PIN_SET)
			{
				updateBalance = 1;
				usr.balance += 5;
			}
		}
		
	}
	// $10 Inserted
	else if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_7) != RESET)
	{
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);
		
		if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_7) == GPIO_PIN_RESET)
		{
			tenDTick = tickCount;
		}
		
		else
		{
			//what about tick rollover since last check? (unlikely, but possible)
			//if(tickCount < tenDTick)		do stuff with this
			
			if(tickCount - tenDTick > BA_LOW_TIME)
			{
				updateBalance = 1;
				usr.balance += 10;
			}
		}
		// Wait 90 ms and make sure pin is still low (low for 100 ms if bill inserted)
		HAL_Delay(90);
		
		if(tickCount - baTick >= 90)
		{
			
		}
		
		baTick = tickCount;
		
		if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_7) == GPIO_PIN_RESET)
		{
			// Wait for 20 more ms and make sure pin went back high
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_7) == GPIO_PIN_SET)
			{
				updateBalance = 1;
				usr.balance += 10;
			}
		}
		
	}
	
	//$20 Inserted
	else if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6) != RESET)
	{
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);
		
		if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_6) == GPIO_PIN_RESET)
		{
			twentyDTick = tickCount;
		}
		
		else
		{
			//what about tick rollover since last check? (unlikely, but possible)
			//if(tickCount < twentyDTick)		do stuff with this
			
			if(tickCount - twentyDTick > BA_LOW_TIME)
			{
				updateBalance = 1;
				usr.balance += 20;
			}
		}
		
		
		// Wait 90 ms and make sure pin is still low (low for 100 ms if bill inserted)
		HAL_Delay(90);
		if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_6) == GPIO_PIN_RESET)
		{
			// Wait for 20 more ms and make sure pin went back high
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_6) == GPIO_PIN_SET)
			{
				updateBalance = 1;
				usr.balance += 20;
			}
		}
	
	}
}

/* Touchscreen and Pushbutton ISR */
void EXTI15_10_IRQHandler(void)
{
	// TS interrupt detected 
	if(__HAL_GPIO_EXTI_GET_IT(TS_INT_PIN) != RESET)
	{
		__HAL_GPIO_EXTI_CLEAR_IT(TS_INT_PIN);
	}
	
	BSP_TS_GetState(&TS_State);
	
	if(TS_State.touchDetected && ((tickCount - lastTouch) >= TOUCH_DEADBAND))
	{
		lastTouch = tickCount;
		
		// Get X and Y position of the touch post calibrated 
		x = TS_State.touchX[0];
		y = TS_State.touchY[0];
		
		//wake device
		if(state == SLEEP)
		{
			state = SELECTION;
			
			//turn off low power mode
			
			
		}
	}
}

uint8_t getItemId(uint8_t selection[])
{
	uint8_t retVal;
	
	switch(selection[0])
	{
		case ((uint8_t) 'A'):
			switch(selection[1])
			{
				case ((uint8_t) '0'):
					retVal=0;
				break;
				case ((uint8_t) '1'):
					retVal=1;
				break;
				case ((uint8_t) '2'):
					retVal=2;
				break;
/*				case ((uint8_t) '3'):
					retVal=3;
				break;
				case ((uint8_t) '4'):
					retVal=4;
				break;*/
				default:
					//error, this should not occur
					retVal=NULL;
			  break;
			}
			break;
		
		case ((uint8_t) 'B'):
			switch(selection[1])
			{
				case ((uint8_t) '0'):
					retVal=3;
				break;
				case ((uint8_t) '1'):
					retVal=4;
				break;
				case ((uint8_t) '2'):
					retVal=5;
				break;
				case ((uint8_t) '3'):
					retVal=6;
				break;
/*				case ((uint8_t) '4'):
					retVal=9;
				break;*/
				default:
					//error
					retVal=NULL;
			  break;
			}
			break;
			
		case ((uint8_t) 'C'):
			switch(selection[1])
			{
				case ((uint8_t) '0'):
					retVal=7;
				break;
				case ((uint8_t) '1'):
					retVal=8;
				break;
				case ((uint8_t) '2'):
					retVal=9;
				break;
				case ((uint8_t) '3'):
					retVal=10;
				break;
/*				case ((uint8_t) '4'):
					retVal=14;
				break;*/
				default:
					//error
					retVal=NULL;
			  break;
			}
			break;
			
		case ((uint8_t) 'D'):
			switch(selection[1])
			{
				case ((uint8_t) '0'):
					retVal=11;
				break;
				case ((uint8_t) '1'):
					retVal=12;
				break;
				case ((uint8_t) '2'):
					retVal=13;
				break;
				case ((uint8_t) '3'):
					retVal=14;
				break;
				case ((uint8_t) '4'):
					retVal=15;
				break;
				default:
					//error
					retVal=NULL;
			  break;
			}
			break;
/*			
		case ((uint8_t) 'E'):
			switch(selection[1])
			{
				case ((uint8_t) '0'):
					retVal=20;
				break;
				case ((uint8_t) '1'):
					retVal=21;
				break;
				case ((uint8_t) '2'):
					retVal=22;
				break;
				case ((uint8_t) '3'):
					retVal=23;
				break;
				case ((uint8_t) '4'):
					retVal=24;
				break;
				default:
					//error
					retVal=NULL;
			  break;
			}
			break;	
*/			
		default:
			//error
			retVal=NULL;
		break;
	}	
	return retVal;
			
}

static void turnMotor(uint8_t motorId)
{
	//Error if motor ID (zero indexed) >= number of motors
	if(motorId >= NUM_MOTORS)
	{
		Error_Handler();
	}
	
	//Will setting F9 trigger interrupt on EXTI9_5? What are repercussions? Disable interrupts?
	
	
	//reset motor select pins in case any are high for some reason
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_8,GPIO_PIN_RESET);

	//enable select lines for motor MUX based on motorId parameter
	if((motorId & 0x01) == 0x01)
	{
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_SET);
	}
	
	else if((motorId & 0x02) == 0x02)
	{
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_SET);
	}
	
	else if((motorId & 0x04) == 0x04)
	{
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_SET);
	}
	
	else if((motorId & 0x08) == 0x08)
	{
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_8,GPIO_PIN_SET);
	}
	
	//set enable line for motors
	HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_SET);
	
	//poor man's PWM (100Hz, 50% dc... interruptable)
	for(uint8_t i = 0; i < NUM_MOTOR_PULSES; i++)
	{
		HAL_Delay(1);
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_SET);
		HAL_Delay(1);
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);
	}
	
	//reset all lines for motors
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_8,GPIO_PIN_RESET);

	HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_RESET);
}


static void motorPinsInit(void)
{
	// Use pins on GPIO Port A 
	GPIO_TypeDef*     gpio_port;
  gpio_port = GPIOA;
 
	// Enable the GPIO Port A Clock 
	__HAL_RCC_GPIOA_CLK_ENABLE();
	// Enable the GPIO Port C Clock 
	__HAL_RCC_GPIOF_CLK_ENABLE();
 	
	// GPIO Configuration Settings
  GPIO_InitTypeDef gpio_init_structure;
  gpio_init_structure.Pull = GPIO_NOPULL;			       // No pull up or pull down resistors
  gpio_init_structure.Speed = GPIO_SPEED_FAST;
  gpio_init_structure.Mode = GPIO_MODE_IT_RISING;   // Rising edge triggered
	
	// Initialize Pin PA0 for motor select 0
	gpio_init_structure.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(gpio_port, &gpio_init_structure);
	
	// Use pins on GPIO Port F
  gpio_port = GPIOF;
	
	// Initialize Pin PF10 for motor select 1
  gpio_init_structure.Pin = GPIO_PIN_10;
  HAL_GPIO_Init(gpio_port, &gpio_init_structure);
		
 	
	// Initialize Pin PF9 for motor select 2
  gpio_init_structure.Pin = GPIO_PIN_9;
  HAL_GPIO_Init(gpio_port, &gpio_init_structure);
	
	// Initialize Pin PF8 for motor select 3
  gpio_init_structure.Pin = GPIO_PIN_8;
  HAL_GPIO_Init(gpio_port, &gpio_init_structure);
	
	// Use pins on GPIO Port G
  gpio_port = GPIOG;
	
	// Initialize Pin PG6 for motor enable
  gpio_init_structure.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(gpio_port, &gpio_init_structure);
	
	// Use pins on GPIO Port B
  gpio_port = GPIOB;
	
	// Initialize Pin PB4 for motor pulse (maybe use a HAL pwm?)
  gpio_init_structure.Pin = GPIO_PIN_4;
  HAL_GPIO_Init(gpio_port, &gpio_init_structure);
}

/**
* @brief  Initialize timer interrupt.
* @param  None
* @retval None
*/
void TIM7_Init() {
	/* TIMx Peripheral clock enable */
	__HAL_RCC_TIM7_CLK_ENABLE();

	/* Set the TIMx priority */
	HAL_NVIC_SetPriority(TIM7_IRQn, 3, 0);

	/* Enable the TIMx global Interrupt */
	HAL_NVIC_EnableIRQ(TIM7_IRQn);

	/* Compute the prescaler value to have TIMx counter clock equal to 1000 Hz */
	uwPrescalerValue = (uint32_t)(SystemCoreClock / 1000) - 1;

	/* Set TIMx instance */
	TimHandle.Instance = TIM7;
	TimHandle.Init.Period            = 10 - 1;
	TimHandle.Init.Prescaler         = uwPrescalerValue;
	TimHandle.Init.ClockDivision     = 0;
	TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
	TimHandle.Init.RepetitionCounter = 0;

	if (HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	/* Start Channel1 */
	if (HAL_TIM_Base_Start_IT(&TimHandle) != HAL_OK)
	{
		/* Starting Error */
		Error_Handler();     // Error handler
	}
}

/**
* @brief  Timer Interrupt Handler.
* @param  None
* @retval None
*/
void TIM7_IRQHandler(void)
{
	if (__HAL_TIM_GET_FLAG(&TimHandle, TIM_FLAG_UPDATE) != RESET)      //In case other interrupts are also running
	{
		//clear flag if it is set
		if (__HAL_TIM_GET_ITSTATUS(&TimHandle, TIM_IT_UPDATE) != RESET)
		{
			__HAL_TIM_CLEAR_FLAG(&TimHandle, TIM_FLAG_UPDATE);
			
			
			tickCount++;
		}
	}
	
}

/**
* @brief  Error handler for system failures.  Blinks LED.
* @param  None
* @retval None
*/
static void Error_Handler(void)
{
	
	BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 95, (uint8_t *)"ERROR", CENTER_MODE);
	BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 80, (uint8_t *)"System Failure.  You will probably die.", CENTER_MODE);
	
	while(1)
	{
		BSP_LED_On(LED1);
		HAL_Delay(1000);
		BSP_LED_Off(LED1);
		HAL_Delay(1000);
	}
}


// Returns cost of slection passed in
float getCost(uint8_t letter,uint8_t number)
{
	float retVal;
	
	switch(letter)
	{
		case ((uint8_t) 'A'):
			switch(number)
			{
				case ((uint8_t) '0'):
					retVal=item[0].itemCost;
				break;
				case ((uint8_t) '1'):
					retVal=item[1].itemCost;
				break;
				case ((uint8_t) '2'):
					retVal=item[2].itemCost;
				break;
				default:
					retVal=0;
			  break;
			}
			break;
		
		case ((uint8_t) 'B'):
			switch(number)
			{
				case ((uint8_t) '0'):
					retVal=item[3].itemCost;
				break;
				case ((uint8_t) '1'):
					retVal=item[4].itemCost;
				break;
				case ((uint8_t) '2'):
					retVal=item[5].itemCost;
				break;
				case ((uint8_t) '3'):
					retVal=item[6].itemCost;
				break;
/*				case ((uint8_t) '4'):
					retVal=2;
				break;*/
				default:
					retVal=0;
			  break;
			}
			break;
			
		case ((uint8_t) 'C'):
			switch(number)
			{
				case ((uint8_t) '0'):
					retVal=item[7].itemCost;
				break;
				case ((uint8_t) '1'):
					retVal=item[8].itemCost;
				break;
				case ((uint8_t) '2'):
					retVal=item[9].itemCost;
				break;
				case ((uint8_t) '3'):
					retVal=item[10].itemCost;
				break;
/*				case ((uint8_t) '4'):
					retVal=3;
				break;*/
				default:
					retVal=0;
			  break;
			}
			break;
			
		case ((uint8_t) 'D'):
			switch(number)
			{
				case ((uint8_t) '0'):
					retVal=item[11].itemCost;
				break;
				case ((uint8_t) '1'):
					retVal=item[12].itemCost;
				break;
				case ((uint8_t) '2'):
					retVal=item[13].itemCost;
				break;
				case ((uint8_t) '3'):
					retVal=item[14].itemCost;
				break;
				case ((uint8_t) '4'):
					retVal=item[15].itemCost;
				break;
				default:
					retVal=0;
			  break;
			}
			break;
/*			
		case ((uint8_t) 'E'):
			switch(number)
			{
				case ((uint8_t) '0'):
					retVal=5;
				break;
				case ((uint8_t) '1'):
					retVal=5;
				break;
				case ((uint8_t) '2'):
					retVal=5;
				break;
				case ((uint8_t) '3'):
					retVal=5;
				break;
				case ((uint8_t) '4'):
					retVal=5;
				break;
				default:
					retVal=0;
			  break;
			}
			break;	
*/			
		default:
			retVal=0;
		break;
	}	
	return retVal;
}
