/* Includes */
#include "main.h"
#include "uart.h"
#include "display.h"
#include "configurations.h"

#define NUM_MOTORS 16			//max number of motors
#define PASSWORD 12345		//temporary, later we will make it available to be changed
#define	TOUCH_DEADBAND 200	//msec delay so multiple touches aren't detected quickly
#define	BA_LOW_TIME 90	//msec delay to determine if a bill has been inserted
#define NUM_MOTOR_PULSES 200	//number of pulses to turn motors
#define NUM_ITEMS		19

/*********************** FAT FS Stuff ************************
FATFS SD_FatFs;  // File system object for SD card logical drive 
char SD_Path[4]; // SD card logical drive path 
char* pDirectoryFiles[MAX_BMP_FILES];
uint8_t  ubNumberOfFiles = 0;
uint32_t uwBmplen = 0;
**************************************************************/

static TS_StateTypeDef  TS_State;
enum STATE{SLEEP,SLIDE_CARD,SELECTION,STOCK,DONE};

/* Global Variables */
uint8_t  status = 0;						// Push button status
volatile uint16_t x, y; 									// x,y location of touch input
float cost = 0.00; 							// Selected item cost
float balance = 0.00;						// Current balance
volatile enum STATE state = SELECTION;   // UI state
enum STATE prevState = SLEEP;   // Previous UI state
uint8_t  text[30] = {0,0};  		// Text buffer for display
uint8_t selectionPressed = NULL;// Input selection
char* received_data;						// Characters recieved from UART transfer
uint8_t selectDisp[2] = {(uint8_t)'_',(uint8_t)'_'};  //display of selection
volatile uint32_t tickCount;		// counter for 1ms timers
volatile uint32_t	lastTouch = 0;		// tick count of last touch detected on touchscreen
volatile uint32_t baTick = 0;				// tick count of last input detected from bill acceptor
volatile uint32_t lastActivityTick = 0;	//tick count of last activity by user (to keep track of when to sleep)

volatile uint32_t oneDTick, fiveDTick, tenDTick,twentyDTick;	//tick count of last $1,$5,$10,$20 input

struct Item
{
	uint8_t itemID;
	uint8_t itemCost;
};

volatile struct Item item[NUM_ITEMS];

TIM_HandleTypeDef TimHandle;
uint32_t uwPrescalerValue;

static void motorPinsInit(void);
uint8_t *motorId = 0;
static uint8_t getMotorId(uint8_t selection[]);
static void turnMotor(uint8_t *motorId);

//States for password characters.  Password must be entered in correct order
enum Pw_State
{
	S0,
	S1,
	S2,
	S3,
	S4,
	S5
} pwState = S0;

/* Flags */
uint8_t updateBalance = 1;	    // Refresh balance display
uint8_t updateCost = 1;					// Refresh cost display Flag
uint8_t updateSelection = 1;	  // Refresh selection display Flag
uint8_t clear = 0;	            // Clear selection Flag
uint8_t admin = 0;							// Admin Flag

void billAcceptorInit(void);
static void TIM7_Init(void);
static void Error_Handler(void);
	
int main(void)
{
	/* Configure the MPU attributes as Write Through */
  MPU_Config();

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
	//motorPinsInit();
	
	//Initiialize Timer 
	TIM7_Init();
	
	/* Clear the LCD */
  clearScreen();
	
  /* Initialize Touchscreen */	
	status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	
	/* Configure TS interrupt */
	BSP_TS_ITConfig();
	
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
		// When touch occurs
		while(state == SELECTION)
		{
			// Draw the UI if state has changed
			if(prevState != SELECTION)
			{
				prevState = SELECTION; // Ensure we dont redraw the UI unless necesarry				
				drawDisplayFrame();		 // Render display frame
				
				//Render Balance area
				BSP_LCD_DisplayStringAt(10, 150, (uint8_t*) "Balance", LEFT_MODE);
				
				drawKeypad();					 // Render selection buttons			
				BSP_LCD_SetBackColor(LCD_COLOR_WHITE);					
			}
			
			/*	NOTE: Make this part more efficient	*/
			
			// Get the cost of the selected item
			cost = getCost(selectDisp[0],selectDisp[1]);
			
			// Display cost in red or green based on balance and the price of the item
			if(balance < cost)
				BSP_LCD_SetTextColor(LCD_COLOR_RED);
			else
				BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			
			// Display cost of selected item
			sprintf((char*)text, "$%0.2f",cost);
			BSP_LCD_DisplayStringAt(10, 115, (uint8_t*) text, LEFT_MODE);											
			BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
			BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
			BSP_LCD_SetFont(&Font24);	
			
			// When valid button pressed display selections
			if(updateSelection == 1)
			{
				sprintf((char*)text, "%c%c",selectDisp[0],selectDisp[1]);
				BSP_LCD_DisplayStringAt(10, 58, (uint8_t*) text, LEFT_MODE);
				updateSelection = 0;
			}
			
			// When money inserted display updated balance			
			if(updateBalance == 1)
			{
				BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
				sprintf((char*)text, "$%0.2f",balance);
				BSP_LCD_DisplayStringAt(10, 172, (uint8_t*) text, LEFT_MODE);
				updateBalance = 0;
			}
			
			// If button press detected
			if(x != 0 && y != 0)
			{
				// Get the Button that was pressed
				selectionPressed = checkButton(&x,&y);
				
				//HAL_Delay(150);
				
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
					/*
					//selection has been made, so turn motors if balance is greater than or equal to cost
					if(selectDisp[0] != (uint8_t)'_' && selectDisp[1] != (uint8_t)'_' && balance >= cost)
					{
						*motorId = getMotorId(selectDisp);
						
						if(*motorId == NULL)
						{
							//do some error crap
							Error_Handler();
							
						}
						
						turn motors
						turnMotor(motorId);
						
					}
					*/
				}
				
				// Clear touch coordinates, selection clear flag and button pressed flag
				x = 0;
				y = 0;
				clear = 0;						
				selectionPressed = NULL;						
			}
		
		}
		
		// Shut down UI and	Enable Low Power Mode
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
			// Draw the UI if state has changed
			if(prevState != STOCK)
			{
				prevState = STOCK; // Ensure we dont redraw the UI unless necesarry				
				drawStockDisplayFrame();		 // Render display area			
				drawPassKeypad();					 // Render passkey buttons			
				BSP_LCD_SetBackColor(LCD_COLOR_WHITE);					
			}
			
				//get password then render rest

			
			
			
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
				balance += 1;
			}
		}
		
		/*
		// Wait 90 ms and make sure pin is still low (low for 100 ms if bill inserted)
		HAL_Delay(90);
		
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) == GPIO_PIN_RESET)
		{
			// Wait for 20 more ms and make sure pin went back high
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) == GPIO_PIN_SET)
			{
				updateBalance = 1;
				balance += 1;
			}
		}
		
		*/
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
				balance += 5;
			}
		}
		
		/*
		// Wait 90 ms and make sure pin is still low (low for 100 ms if bill inserted)
		HAL_Delay(90);
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8) == GPIO_PIN_RESET)
		{
			// Wait for 20 more ms and make sure pin went back high
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8) == GPIO_PIN_SET)
			{
				updateBalance = 1;
				balance += 5;
			}
		}
		*/
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
				balance += 10;
			}
		}
		// Wait 90 ms and make sure pin is still low (low for 100 ms if bill inserted)
		/*HAL_Delay(90);
		
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
				balance += 10;
			}
		}
		*/
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
				balance += 20;
			}
		}
		
		/*
		// Wait 90 ms and make sure pin is still low (low for 100 ms if bill inserted)
		HAL_Delay(90);
		if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_6) == GPIO_PIN_RESET)
		{
			// Wait for 20 more ms and make sure pin went back high
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_6) == GPIO_PIN_SET)
			{
				updateBalance = 1;
				balance += 20;
			}
		}
	*/
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

uint8_t getMotorId(uint8_t selection[])
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
				case ((uint8_t) '3'):
					retVal=3;
				break;
				case ((uint8_t) '4'):
					retVal=4;
				break;
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
					retVal=5;
				break;
				case ((uint8_t) '1'):
					retVal=6;
				break;
				case ((uint8_t) '2'):
					retVal=7;
				break;
				case ((uint8_t) '3'):
					retVal=8;
				break;
				case ((uint8_t) '4'):
					retVal=9;
				break;
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
					retVal=10;
				break;
				case ((uint8_t) '1'):
					retVal=11;
				break;
				case ((uint8_t) '2'):
					retVal=12;
				break;
				case ((uint8_t) '3'):
					retVal=13;
				break;
				case ((uint8_t) '4'):
					retVal=14;
				break;
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
					retVal=15;
				break;
				case ((uint8_t) '1'):
					retVal=16;
				break;
				case ((uint8_t) '2'):
					retVal=17;
				break;
				case ((uint8_t) '3'):
					retVal=18;
				break;
				case ((uint8_t) '4'):
					retVal=19;
				break;
				default:
					//error
					retVal=NULL;
			  break;
			}
			break;
			
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
			
		default:
			//error
			retVal=NULL;
		break;
	}	
	return retVal;
			
}

static void turnMotor(uint8_t *motorId)
{
	//Error if motor ID (zero indexed) >= number of motors
	if(*motorId >= NUM_MOTORS)
	{
		Error_Handler();
	}
	
	//disable interrupts?  Will setting F9 trigger interrupt on EXTI9_5?
	
	//reset motor select pins in case any are high for some reason
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_8,GPIO_PIN_RESET);

	//enable select lines for motor MUX based on motorId parameter
	if((*motorId & 0x01) == 0x01)
	{
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_SET);
	}
	
	if((*motorId & 0x02) == 0x02)
	{
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_SET);
	}
	
	if((*motorId & 0x04) == 0x04)
	{
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_SET);
	}
	
	if((*motorId & 0x08) == 0x08)
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
			
			//rollover counter
			//if(tickCount++ >= sizeof(uint32_t)) //skips the last tick in 32bits, but who cares
			//{
				tickCount++;// = 0;
			//}
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
