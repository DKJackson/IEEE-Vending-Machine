/* Define to prevent recursive inclusion */
#ifndef __DISPLAY_H
#define __DISPLAY_H

/* Includes */
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include <stdlib.h>

#define  BUTTON_SIZE          50
#define  BUTTON_XPOS(j)       (200+((BUTTON_SIZE+10)*j))  //x position for buttons
#define  BUTTON_YPOS(i)       (50+((BUTTON_SIZE+10)*i)) //y position for buttons

#define  SELECTION_SCREEN			1 //screen definition for item selection
#define  DIGIT_SCREEN					2 //screen definition for slide card

uint8_t dispText[30];
uint8_t currScreen = DIGIT_SCREEN;

/* Exported functions */	 
void LCD_Configuration(void);
void clearScreen(void);
void drawButton(uint16_t xPos, uint16_t yPos, uint8_t *label, uint32_t color, uint8_t buttonSize);
void drawDisplayFrame(void);
void drawDigits(void);

//void drawAdmin(void);
void drawAdminDispFrame(void);
void displayCost(uint8_t s0,uint8_t s1);
void drawCardFrame(void);
void updateCostDisp(float usrBalance, float itemCost, uint8_t updateZone);
void updateBalanceDisp(float usrBalance);
void updateSelectionDisp(uint8_t s1, uint8_t s2, uint8_t updateZone);
void showBalanceArea(void);
void showItemCountArea(void);
void updateItemCountDisp(uint8_t cnt);
void drawCardFrame(void);
#endif 

