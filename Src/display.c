/* Includes ------------------------------------------------------------------*/
#include "display.h"

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

void drawCardFrame(void)

{
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

	drawDigits();

	return;
}

void updateCostDisp(float usrBalance, float itemCost, uint8_t updateZone)
{
	if(updateZone)
	{
		BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGREEN);
	}
	
	else
	{
		BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
	}
	
	// Display cost in red or green based on balance and the price of the item
	if(usrBalance < itemCost)
		BSP_LCD_SetTextColor(LCD_COLOR_RED);
	else
		BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
	
	BSP_LCD_SetFont(&Font24);
	
	// Display cost of selected item
	sprintf((char*) dispText, "$%0.2f",itemCost);
	BSP_LCD_DisplayStringAt(10, 115, (uint8_t*) dispText, LEFT_MODE);
}

void updateBalanceDisp(float usrBalance)
{
	BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	sprintf((char*)dispText, "$%0.2f",usrBalance);
	BSP_LCD_DisplayStringAt(10, 172, (uint8_t*) dispText, LEFT_MODE);
}

void updateSelectionDisp(uint8_t s1, uint8_t s2, uint8_t updateZone)
{
	if(updateZone)
	{
		BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGREEN);
	}
	
	else
	{
		BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
	}
	
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	sprintf((char*)dispText, "%c%c",s1,s2);
	BSP_LCD_DisplayStringAt(10, 58, (uint8_t*) dispText, LEFT_MODE);
}

void showBalanceArea(void)
{
	BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_DisplayStringAt(10, 150, (uint8_t*) "Balance", LEFT_MODE);
}

void showItemCountArea(void)
{
	BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_DisplayStringAt(10, 150, (uint8_t*) "Count", LEFT_MODE);
}


void updateItemCountDisp(uint8_t cnt)
{
	BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	sprintf((char*)dispText, "%d",cnt);
	BSP_LCD_DisplayStringAt(10, 172, (uint8_t*) dispText, LEFT_MODE);
}