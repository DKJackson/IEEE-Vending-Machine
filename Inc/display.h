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
#define  PASS_BUTTON_SIZE			30
#define  BUTTON_XPOS(j)       (200+((BUTTON_SIZE+10)*j))  //x position for buttons
#define  BUTTON_YPOS(i)       (50+((BUTTON_SIZE+10)*i)) //y position for buttons
#define  PASS_BUTTON_XPOS(j)  (48+((PASS_BUTTON_SIZE+5)*j))	//x position for passkey buttons
#define  PASS_BUTTON_YPOS(i)  (152+((PASS_BUTTON_SIZE+5)*i))	//y position for passkey buttons

#define A_BUTTON_XPOS		20
#define A_BUTTON_YPOS		(BSP_LCD_GetYSize() - 40)

/* Exported functions */	 
void LCD_Configuration(void);
void clearScreen(void);
void drawButton(uint16_t xPos, uint16_t yPos, uint8_t *label, uint32_t color, uint8_t buttonSize);
uint8_t checkButton(uint16_t xCoord, uint16_t yCoord, uint8_t screen);
uint8_t getButtonDef(uint8_t row,uint8_t column, uint8_t screen);
void drawKeypad(void);
void drawDisplayFrame(void);
float getCost(uint8_t letter,uint8_t number);
uint8_t validateSelection(uint8_t selectionPressed, uint8_t selectionIndx);
//void drawAdmin(void);
void drawAdminDispFrame(void);
void drawPassKeypad(void);
uint8_t getPasskeyButtonDef(uint8_t row,uint8_t column);
uint8_t checkPassKeyButton(volatile uint16_t xCoord,volatile uint16_t yCoord);
void displayCost(uint8_t s0,uint8_t s1);
void drawCardFrame(void);
#endif 

