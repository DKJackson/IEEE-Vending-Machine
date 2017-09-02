/* Includes ------------------------------------------------------------------*/
#include "display.h"
void drawCardFrame(void);

/**
  * @brief  LCD configuration
  * @param  None
  * @retval None
  */
void LCD_Configuration(void)
{
  /* LCD Initialization */ 
  BSP_LCD_Init();

  /* LCD Initialization */ 
  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
  BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS+(BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4));

  /* Enable the LCD */ 
  BSP_LCD_DisplayOn(); 
  
  /* Select the LCD Background Layer  */
  BSP_LCD_SelectLayer(0);

  /* Clear the Background Layer */ 
  BSP_LCD_Clear(LCD_COLOR_BLACK);  
  
  /* Select the LCD Foreground Layer  */
  BSP_LCD_SelectLayer(1);

  /* Clear the Foreground Layer */ 
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  
  /* Configure the transparency for foreground and background :
     Increase the transparency */
  BSP_LCD_SetTransparency(0, 0);
  BSP_LCD_SetTransparency(1, 200);
	return;
}


/**
  * @brief Clears LCD screen
	*
	* @param None
	* @retval None
  */
void clearScreen(void)
{
	BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_Clear(LCD_COLOR_WHITE);
}

/**
  * @brief Draws button
	*
	* @param x position of button center
	* @param y position of button center
	* @param button text label
	* @param button color
	* @retval None
  */ 
void drawButton(uint16_t xPos, uint16_t yPos, uint8_t *label, uint32_t color, uint8_t buttonSize)
{
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_FillRect(xPos-buttonSize/2-2, yPos-buttonSize/2-2, buttonSize+2, buttonSize+2);
	BSP_LCD_SetTextColor(color);
	BSP_LCD_FillRect(xPos-buttonSize/2, yPos-buttonSize/2, buttonSize-2, buttonSize-2);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(color);
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_DisplayStringAt(xPos-9, yPos-11, label, LEFT_MODE);
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
	// Changed this to column and row (more understandable for reader)
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
						retVal=(uint8_t)'A';
					break;
					case 1:
						retVal=(uint8_t)'B';
					break;
					case 2:
						retVal=(uint8_t)'C';
					break;
					case 3:
						retVal=(uint8_t)'D';
					break;
					case 4:
						retVal=(uint8_t)'E';
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
						retVal=(uint8_t)'0';
					break;
					case 1:
						retVal=(uint8_t)'1';
					break;
					case 2:
						retVal=(uint8_t)'2';
					break;
					case 3:
						retVal=(uint8_t)'3';
					break;
					case 4:
						retVal=(uint8_t)'4';
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
						retVal=(uint8_t)'X';
					break;
					case 1:
						retVal=(uint8_t)'<';
					break;
					case 4:
						retVal=(uint8_t)'>';
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
	
	// screen = 2 is the card screen
	if(screen == 2)
	{
		switch(row)
		{

			case 0:
				switch(column)
				{

					case 0:
						retVal=(uint8_t)'0';
					break;
					case 1:
						retVal=(uint8_t)'1';
					break;
					case 2:
						retVal=(uint8_t)'2';
					break;
					case 3:
						retVal=(uint8_t)'3';
					break;
					case 4:
						retVal=(uint8_t)'4';
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
						retVal=(uint8_t)'5';
					break;
					case 1:
						retVal=(uint8_t)'6';
					break;
					case 2:

						retVal=(uint8_t)'7';
					break;
					case 3:
						retVal=(uint8_t)'8';
					break;
					case 4:
						retVal=(uint8_t)'9';
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
						retVal=(uint8_t)'X';
					break;
					case 1:
						retVal=(uint8_t)'<';
					break;
					case 4:
						retVal=(uint8_t)'>';
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

void drawKeypad(void)
{
	uint8_t  temp[30] = {0,0};  		// Text buffer for display
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
				sprintf((char*)temp, "%c",getButtonDef(row,column,1));
				drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row), (uint8_t*)temp, LCD_COLOR_BLUE, (uint8_t) BUTTON_SIZE);
			}
			// Special buttons on row 3
			else if(column != 2 && column != 3)
			{			
				sprintf((char*)temp, "%c",getButtonDef(row,column,1));
				
				switch(column)
				{
					case 0:
						drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row),(uint8_t*) temp, LCD_COLOR_RED, BUTTON_SIZE);
						break;
					
					case 1:
						drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row),(uint8_t*) temp, LCD_COLOR_YELLOW,BUTTON_SIZE);
						break;
					
					case 4:
						drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row),(uint8_t*) temp, LCD_COLOR_GREEN,BUTTON_SIZE);
				}
			}
		}
	}
}

void drawDisplayFrame(void)
{
	//Makes the left side a different color background
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	//BSP_LCD_FillRect(0, 0, 167, BSP_LCD_GetYSize());
	BSP_LCD_DrawLine(166, 0, 166, BSP_LCD_GetYSize());
	BSP_LCD_DrawLine(165, 0, 165, BSP_LCD_GetYSize());
	
	BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_FillRect(0, 0, 165, BSP_LCD_GetYSize());
	
	//set text font and color
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_SetFont(&Font24);
	
	//Render Selection area
	BSP_LCD_DisplayStringAt(10, 30, (uint8_t*) "Selection", LEFT_MODE);
	
	//Render Cost area
	BSP_LCD_DisplayStringAt(10, 90, (uint8_t*) "Cost", LEFT_MODE);
	
}

/*
uint8_t getCost(uint8_t letter,uint8_t number)
{
	uint8_t retVal;
	
	switch(letter)
	{
		case ((uint8_t) 'A'):
			switch(number)
			{
				case ((uint8_t) '0'):
					retVal=1;
				break;
				case ((uint8_t) '1'):
					retVal=1;
				break;
				case ((uint8_t) '2'):
					retVal=1;
				break;
				case ((uint8_t) '3'):
					retVal=1;
				break;

				case ((uint8_t) '4'):
					retVal=1;
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
					retVal=2;
				break;
				case ((uint8_t) '1'):
					retVal=2;
				break;
				case ((uint8_t) '2'):
					retVal=2;
				break;
				case ((uint8_t) '3'):
					retVal=2;
				break;
				case ((uint8_t) '4'):
					retVal=2;
				break;
				default:
					retVal=0;
			  break;
			}
			break;
			

		case ((uint8_t) 'C'):
			switch(number)
			{
				case ((uint8_t) '0'):
					retVal=3;
				break;
				case ((uint8_t) '1'):
					retVal=3;
				break;
				case ((uint8_t) '2'):
					retVal=3;
				break;
				case ((uint8_t) '3'):
					retVal=3;
				break;
				case ((uint8_t) '4'):
					retVal=3;
				break;
				default:
					retVal=0;
			  break;
			}
			break;
			
		case ((uint8_t) 'D'):
			switch(number)
			{
				case ((uint8_t) '0'):
					retVal=4;
				break;
				case ((uint8_t) '1'):
					retVal=4;
				break;
				case ((uint8_t) '2'):
					retVal=4;
				break;
				case ((uint8_t) '3'):
					retVal=4;
				break;
				case ((uint8_t) '4'):
					retVal=4;
				break;
				default:
					retVal=0;
			  break;

			}
			break;
			
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
			
		default:
			retVal=0;
		break;
	}
	
	return retVal;
}
*/
uint8_t validateSelection(uint8_t selectionPressed, uint8_t selectionIndx)
{
	if(selectionIndx == 0)
	{
		switch(selectionPressed)
		{
			case (uint8_t)'A':
			case (uint8_t)'B':
			case (uint8_t)'C':
			case (uint8_t)'D':
			case (uint8_t)'E':
				return 1;
			default:
				break;
		}
	}
	
	else if(selectionIndx == 1)
	{
		switch(selectionPressed)
		{
			case (uint8_t)'0':
			case (uint8_t)'1':
			case (uint8_t)'2':
			case (uint8_t)'3':
			case (uint8_t)'4':
				return 1;
			default:
				break;
		}
	}
	
	return 0;
}

void drawAdminDispFrame(void)
{
	clearScreen();
	
	drawDisplayFrame();
	
	//Render Selection area
	BSP_LCD_DisplayStringAt(80, 10, (uint8_t*) "Enter Passcode", CENTER_MODE);
	
	drawPassKeypad();
	
	
	
}

void drawPassKeypad(void)
{
	uint8_t temp[30] = {0,0};  //holds value of button
	
	for(uint8_t row = 0; row < 4; row++)
	{
		for(uint8_t column = 0; column < 3; column++)
		{
			if(row ==3 && column != 1)
			{
				continue;
			}
			sprintf((char*)temp, "%c",getPasskeyButtonDef(row,column));
			//sprintf((char*)temp, "%c",i);
			drawButton(PASS_BUTTON_XPOS(column),PASS_BUTTON_YPOS(row), (uint8_t*)temp, LCD_COLOR_BLUE,PASS_BUTTON_SIZE);
		}
	}
}

uint8_t checkPassKeyButton(volatile uint16_t xCoord,volatile uint16_t yCoord)
{
	// Changed this to column and row (more understandable for reader)
	//uint8_t iter[2] = {0, 0}; //hold iterator values for i and j at touch detection location to determine button
	uint8_t column = 0;
	uint8_t row = 0;
	
	// iterate through columns and rows to determine which button was pressed
	// eg. col=0 and row=0 indicates that the button labeled "1" was pressed
	for(uint8_t i = 0; i <= 4; i++)
	{
		for(uint8_t j = 0; j <=3; j++)
		{
			if(xCoord > PASS_BUTTON_XPOS(j)-PASS_BUTTON_SIZE/2 && yCoord > PASS_BUTTON_YPOS(i)-PASS_BUTTON_SIZE/2 && xCoord < PASS_BUTTON_XPOS(j)+PASS_BUTTON_SIZE/2 && yCoord < PASS_BUTTON_YPOS(i)+PASS_BUTTON_SIZE/2)
			{
				column = i;
				row = j;
				return getPasskeyButtonDef(row,column);
			}
		}
	}
	return NULL;
}

/*
void drawAdmin(void)
{
	BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
	BSP_LCD_SetFont(&Font24);
	clearScreen();
	// Draw Green  Yes Button
	BSP_LCD_DisplayStringAt(110, 120, (uint8_t*) "CONTINUE AS ADMIN", LEFT_MODE);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_FillRect(170-29, 210-29, 58, 58);
	BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
	BSP_LCD_FillRect(170-27, 210-27, 54, 54);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_GREEN);
	BSP_LCD_DisplayStringAt(145, 200, (uint8_t*) "YES", LEFT_MODE);
	// Draw Red No Button
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_FillRect(300-29, 210-29, 58, 58);
	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	BSP_LCD_FillRect(300-27, 210-27, 54, 54);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(283, 200, (uint8_t*) "NO", LEFT_MODE);
	return;
}
*/

/**
  * @brief Returns character value (as an 8 bit integer) of the pressed button
	*				
	* @param button row
	* @param button column
	* @retval Value of button pressed
  */
uint8_t getPasskeyButtonDef(uint8_t row,uint8_t column)
{
	uint8_t retVal;	//value to return
	
	//button rows and keys are similar to regular keyads
	switch(row)
	{
		case 0:
			switch(column)
			{
				case 0:
					retVal=(uint8_t)'1';
				break;
				case 1:
					retVal=(uint8_t)'2';
				break;
				case 2:
					retVal=(uint8_t)'3';
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
					retVal=(uint8_t)'4';
				break;
				case 1:
					retVal=(uint8_t)'5';
				break;
				case 2:
					retVal=(uint8_t)'6';
				break;
				default:
					retVal=NULL;
			  break;
			}
			break;

		case 2:
			switch(column)
			{
				case 0:
					retVal=(uint8_t)'7';
				break;
				case 1:
					retVal=(uint8_t)'8';
				break;
				case 2:
					retVal=(uint8_t)'9';
				break;
				default:
					retVal=NULL;
			  break;
			}
			break;

		case 3:
			switch(column)
			{
/*				case 0:
					retVal=(uint8_t)'<';
				break;*/
				case 1:
					retVal=(uint8_t)'0';
				break;
/*				case 2:
					retVal=(uint8_t)'>';
				break;*/
				default:
				retVal=NULL;
			  break;
			}
			break;
		default:
			retVal=NULL;
		break;
	}
	
	return retVal;
}


void drawCardFrame(void)

{
	uint8_t  temp[30] = {0,0};  		// Text buffer for display
	
	//Makes the left side a different color background
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	BSP_LCD_FillRect(0, 0, 167, BSP_LCD_GetYSize());
	
	BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_FillRect(0, 0, 165, BSP_LCD_GetYSize());
	
	//set text font and color
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_SetFont(&Font24);
	
	//Render Card Number area
	BSP_LCD_DisplayStringAt(10, 30, (uint8_t*) "  Enter", LEFT_MODE);
	BSP_LCD_DisplayStringAt(10, 55, (uint8_t*) "StudentID", LEFT_MODE);
	BSP_LCD_DisplayStringAt(0, 80, (uint8_t*) "  Number", LEFT_MODE);

	
	for(uint8_t row = 0; row <= 3; row++)
	{
		// No buttons on row 2
		if(row == 2)
			continue;
		
		for(uint8_t column = 0; column <= 4; column++)
		{
			// Regular buttons on row 0 and 1
			if(row != 3)
			{
				sprintf((char*)temp, "%c",getButtonDef(row,column, 2));
				drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row), (uint8_t*)temp, LCD_COLOR_BLUE, BUTTON_SIZE);

			}
			// Special buttons on row 3
			else if(column != 2 && column != 3)
			{			
				sprintf((char*)temp, "%c",getButtonDef(row,column,2));
				
				switch(column)
				{

					case 0:
						drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row),(uint8_t*) temp, LCD_COLOR_RED, BUTTON_SIZE);

						break;
					
					case 1:
						drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row),(uint8_t*) temp, LCD_COLOR_YELLOW, BUTTON_SIZE);

						break;
					
					case 4:
						drawButton(BUTTON_XPOS(column),BUTTON_YPOS(row),(uint8_t*) temp, LCD_COLOR_GREEN, BUTTON_SIZE);
				}
			}
		}
	}

	return;
}
void displayCost(uint8_t s0,uint8_t s1)
{
	
}
