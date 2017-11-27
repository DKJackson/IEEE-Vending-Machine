/* Includes */
#include "main.h"
#include "uart.h"
#include "display.h"
#include "configurations.h"
#include "database.h"
#include <math.h>

	
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
	
	/* Allocate memory for text */
	textInit();
	
  /* Initialize Touchscreen */	
	status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	
	/* Configure TS interrupt */
	BSP_TS_ITConfig();
	
	/* Initialize the Fat File System Database on the SD card */
	databaseInit();
	
	/* Initialize adminState structs */
	adminStateInit();
	
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
			}
			
			/***********************************UPDATE SELECTION***********************************/
			
			if(x != 0 && y != 0)
			{
				// Get the Button that was pressed
				selectionPressed = checkButton(x,y,DIGIT_SCREEN);
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
				if(!clear)
				{
					for(int i=0; i <= 8; i++)
					{
						if(selectionPressed == (uint8_t) '>')
							break;
						if(selectDisp[i] == (uint8_t)'_')
						{
							selectDisp[i] = (selectionPressed == NULL) ? (uint8_t)'_' : selectionPressed;
							updateSelection = 1;
							break;
						}
					}
				}
				
				// Clear touch coordinates and button pressed flag
				x = 0;
				y = 0;
				
				// Go to item selection
				if(selectDisp[8] != (uint8_t) '_' && selectionPressed == (uint8_t) '>')
				{
					state = SELECTION;
					
					/********************************* Get user data and update usr struct************************************/
					
					
					
					
					
					// Setup the name of the file which can only be 8 characters + ".TXT"
					fileName = (char*)&selectDisp;
					strncat (fileName, ".TXT", 4);
					//writeUser((uint8_t*)amount, (char*) fileName);
					//amountOnCard = readUser((char*) fileName);
					// File Does Not Exist
/*					if(amountOnCard == (uint8_t*)'X')
					{
						usr->balance += 0;
					}
*/
					usr =  readUser((char*) fileName);
					
					// Find out how much money is on the account
					
					if(!usr)
					{
						Error_Handler();
					}
/*					
					else
					{
						for(int i=3 ; i >= 0; i--)
						{
							if(amountOnCard[i] != NULL)
							{
								usr->balance += (pow(10,j) * (amountOnCard[i] - 48));
								j +=1;
							}
						}
					}
*/
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
			if(updateSelection)
			{
				sprintf(text, "%s",selectDisp);
				BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
				BSP_LCD_DisplayStringAt(5, 120, (uint8_t*) text, LEFT_MODE);
			}
		}
		
		// When touch occurs
		while(state == SELECTION)
		{
			//no user selected
			if(!usr)
			{
				state = SLIDE_CARD;
				prevState = SELECTION;
				continue;
			}
			
			// Draw the UI if state has changed
			if(prevState != SELECTION)
			{
				selectDisp = "_________";
				prevState = SELECTION; // Ensure we dont redraw the UI unless necesarry				
				drawDisplayFrame();		 // Render display frame
				currAdminState = admin[0];	// Reset admin passkey state
				showBalanceArea();	// Render user balance display
				currScreen = SELECTION_SCREEN;	// Set current screen for input buttons
				drawKeypad(currScreen);					 // Render selection buttons
				
				updateBalance = 1;
				updateCost = 1;
				updateSelection = 1;
			}
			
			// Get the cost of the selected item
			if(updateCost)
			{
				if(usr && item)
				{
					updateCostDisp(usr->balance,item->itemCost,0);
				}
				
				else
				{
					updateCostDisp(0.00,0.00,0);
				}
				
				updateCost = 0;
			}
			
			// Display updated balance			
			if(updateBalance)
			{
				if(usr)
				{
					updateBalanceDisp(usr->balance);
				}
				
				else
				{
					updateBalanceDisp(0.00);
				}
				
				updateBalance = 0;
			}
			
			// When valid button pressed display selections
			if(updateSelection)
			{
				updateSelectionDisp(selectDisp[0],selectDisp[1], 0);
				
				if(selectDisp[0] != '_' && selectDisp[1] != '_')
				{
					sprintf(fileName,"%c%c",selectDisp[0],selectDisp[1]);
					item = readItem(fileName);	//retrieve item info from storage
				}
				
				else
				{
					item = NULL;
				}
				
				updateSelection = 0;
			}
			
			// If button press detected
			if(x != 0 && y != 0)
			{
				// Get the Button that was pressed
				selectionPressed = checkButton(x,y,currScreen);
				
				// Delete last character of selection
				if(selectionPressed == '<')
				{
					if(selectDisp[1] != '_')
					{
						selectDisp[1] = '_';
					}

					else if(selectDisp[0] != '_')
					{
						selectDisp[0] = '_';
					}

					item = NULL;
					updateSelection = 1;
					updateCost = 1;
					
					currAdminState = admin[0];
					
					HAL_Delay(100);
				}
				
				// Return to slide card screen
				else if(selectionPressed == (uint8_t) 'X')
				{
					selectDisp = "_________";
					currAdminState = admin[0];
					item = NULL;
					usr = NULL;
					state = SLIDE_CARD;
					prevState = SELECTION;
					selectionPressed = NULL;
					
					continue;
				}
				
				// Vend Item if available and sufficient funds
				else if(selectionPressed == '>')
				{
					currAdminState = admin[0];
					
					// Valid Selection
					if(usr && item)
					{
						if(usr->balance >= item->itemCost && item->itemCount > 0)
						{
							
							//get id of motor to turn
							motorId = item->itemID;
							
							// Vend item
							usr->balance -= item->itemCost;
							updateBalance = 1;
							item->itemCount--;
							
							//turn motors
							turnMotor(motorId);
							
							// update user and item files
							sprintf(fileName,"%d",usr->studentID);
							writeUser(usr, fileName);
							sprintf(fileName,"%d",item->itemID);
							writeItem(item, fileName);
							
							item = NULL;
							updateSelection = 1;
						}
						
						else if(item->itemCount <= 0)
						{
							//display "Try another selection."
							text = "Item out of stock.";
							
							
							
							text = "Try another selection.";
							
							
							
						}
						
						else if(usr->balance < item->itemCost)
						{
							text = "Add more money to account.";
							
							
							
							
						}
					}
					
					selectDisp = "_________";
				}
				
				else
				{
					//check admin passkey
					if(selectionPressed == currAdminState->currKey)
					{
						currAdminState = currAdminState->nextAdminState;
						
						if(currAdminState->access)
						{
							state = STOCK;
							selectionPressed = NULL;
							continue;
						}
					}
						
					// Validate that selection is valid for the selection place
					// ie selectDisp[0] must be A-E selectDisp[1] must be 0-4
					if(selectDisp[0] == '_')
					{
						selectionPressed = validateSelection(selectionPressed,0) ? selectionPressed : NULL;
						selectDisp[0] = (selectionPressed == NULL) ? '_' : selectionPressed;
						updateSelection = 1;
					}
					
					else if(selectDisp[1] == '_')
					{
						selectionPressed = validateSelection(selectionPressed,1) ? selectionPressed : NULL;
						selectDisp[1] = (selectionPressed == NULL) ? '_' : selectionPressed;
						updateSelection = 1;
					}
				}
				
				selectionPressed = NULL;
				
				x = 0;
				y = 0;
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
			
			if(!usr || usr->userType != ADMIN)
			{
				state = SELECTION;
				continue;
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
				usr->balance += 1;
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
				usr->balance += 1;
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
				usr->balance += 5;
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
				usr->balance += 5;
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
				usr->balance += 10;
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
				usr->balance += 10;
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
				usr->balance += 20;
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
				usr->balance += 20;
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

void adminStateInit()
{
	for(uint8_t i = 0; i < ADMIN_PASS_SIZE; i++)
	{
		admin[i]->access = 0;
		admin[i]->currKey = adminPass[i];
		admin[i]->nextAdminState = admin[i+1];
	}
	
	admin[ADMIN_PASS_SIZE]->access = 1;
	admin[ADMIN_PASS_SIZE]->currKey = NULL;
	admin[ADMIN_PASS_SIZE]->nextAdminState = admin[0];
	
	currAdminState = admin[0];
}

void textInit(void)
{
	fileName = (char *)malloc((FILENAME_SIZE + 1) * sizeof(char)); // allocate memory for filename (num characters + null terminator)
	selectDisp = (char *)malloc((SELECTDISP_SIZE + 1) * sizeof(char));  //allocate memory for selection display
	text = (char*)malloc(TEXT_SIZE * sizeof(char));	// allocate memory for text buffer
	item->selectionNum = (char*)malloc(SELECTION_SIZE * sizeof(char));	// allocate memory for item selection number
	dispText = (char*)malloc(TEXT_SIZE * sizeof(char));// allocate memory for text buffer
}

/**
  * @brief Checks which button was pressed
	*
	* @param x coordinate
	* @param y coordinate
  * @param screen selection
	* @retval Value of button pressed or null if not within button bounds
  */
uint8_t checkButton(uint16_t xCoord, uint16_t yCoord, uint8_t screen)
{
	//uint8_t iter[2] = {0, 0}; //hold iterator values for i and j at touch detection location to determine button
	uint8_t column = 0;
	uint8_t row = 0;
	
	// iterate through columns and rows to determine which button was pressed
	// eg. i=0 and j=0 indicates that the button labeled "A" was pressed
	for(uint8_t i = 0; i <= 3; i++)
	{
		for(uint8_t j = 0; j <=4; j++)
		{
			if(xCoord > BUTTON_XPOS(j) - BUTTON_SIZE/2 && yCoord > BUTTON_YPOS(i) - BUTTON_SIZE/2 && 
				xCoord < BUTTON_XPOS(j) + BUTTON_SIZE/2 && yCoord < BUTTON_YPOS(i) + BUTTON_SIZE/2)
			{
				column = i;
				row = j;
				
				return getButtonDef(column,row,screen);
			}
		}
	}
	
	return NULL;
}

/**
  * @brief Returns string value (as an 8 bit integer) of the pressed button
	*
	* @param column pointer to key column no
	* @param row pointer to key row no
	* @param current view
	* @retval Value of button pressed
  */
uint8_t getButtonDef(uint8_t row,uint8_t column,uint8_t screen)
{
	uint8_t retVal;
	// Screen = 1 is the Selection Screen
	if(screen == 1)
	{
		switch(row)
		{
			case 0:
				switch(column)
				{

					case 0:
						retVal='A';
					break;
					case 1:
						retVal='B';
					break;
					case 2:
						retVal='C';
					break;
					case 3:
						retVal='D';
					break;
					case 4:
						retVal='E';
					break;
					default:
					retVal=NULL;
					break;
				}
				break;
			
			case 1:
				switch(column)
				{
					case 0:
						retVal='0';
					break;
					case 1:
						retVal='1';
					break;
					case 2:
						retVal='2';
					break;
					case 3:
						retVal='3';
					break;
					case 4:
						retVal='4';
					break;
					default:
					retVal=NULL;
					break;
				}
				break;

			case 3:
				switch(column)
				{
					case 0:
						retVal='X';
					break;
					case 1:
						retVal='<';
					break;
					case 4:
						retVal='>';
					break;
					default:
						retVal=NULL;
					break;
				}
				break;

			default:
				retVal=NULL;


			break;
		}
	}
	
	// screen = 2 is the digit screen
	if(screen == 2)
	{
		switch(row)
		{

			case 0:
				switch(column)
				{
					case 0:
						retVal='0';
					break;
					case 1:
						retVal='1';
					break;
					case 2:
						retVal='2';
					break;
					case 3:
						retVal='3';
					break;
					case 4:
						retVal='4';
					break;
					default:
					retVal=NULL;
					break;
				}
				break;
			
			case 1:
				switch(column)
				{
					case 0:
						retVal='5';
					break;
					case 1:
						retVal='6';
					break;
					case 2:

						retVal='7';
					break;
					case 3:
						retVal='8';
					break;
					case 4:
						retVal='9';
					break;
					default:
					retVal=NULL;
					break;
				}
				break;
				
			case 3:
				switch(column)
				{
					case 0:
						retVal='X';
					break;
					case 1:
						retVal='<';
					break;
					case 4:
						retVal='>';
					break;
					default:
					retVal=NULL;
					break;
				}
				break;		
			default:
				retVal=NULL;


			break;
		}
	}

	return retVal;
}

uint8_t validateSelection(char selectionPressed, uint8_t selectionIndx)
{
	if(selectionIndx == 0)
	{
		switch(selectionPressed)
		{
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
				return 1;
			default:
				break;
		}
	}
	
	else if(selectionIndx == 1)
	{
		switch(selectionPressed)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
				return 1;
			default:
				break;
		}
	}
	
	return 0;
}


void drawKeypad(uint8_t screen)
{
	for(uint8_t row = 0; row <= 3; row++)
	{
		// No buttons on row 2
		if(row == 2)
			continue;
		
		for(uint8_t column = 0; column <= 4; column++)
		{
			//don't display last button in first row
//			if(column == 4 && row == 0)
//				continue;
			
			// Regular buttons on row 0 and 1
			if(row != 3)
			{
				sprintf((char*)dispText, "%c",getButtonDef(row,column,screen));
				drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row), dispText, LCD_COLOR_BLUE, BUTTON_SIZE);
			}
			// Special buttons on row 3
			else if(column != 2 && column != 3)
			{			
				sprintf((char*)dispText, "%c",getButtonDef(row,column,screen));
				
				switch(column)
				{
					case 0:
						drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row),dispText, LCD_COLOR_RED, BUTTON_SIZE);
						break;
					
					case 1:
						drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row),dispText, LCD_COLOR_YELLOW,BUTTON_SIZE);
						break;
					
					case 4:
						drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row),dispText, LCD_COLOR_GREEN,BUTTON_SIZE);
				}
			}
		}
	}
}