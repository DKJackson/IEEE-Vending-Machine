/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "database.h"

void databaseInit(void);
static void Error_Handler(void);
void writeFile(uint8_t *text, char *fileName);
uint8_t* readFile(char *fileName);

FATFS SDFatFs;  /* File system object for SD card logical drive */
FIL MyFile;     /* File object */
char SDPath[4]; /* SD card logical drive path */

void databaseInit(void)
{    
  /* Link the micro SD disk I/O driver */
  if(FATFS_LinkDriver(&SD_Driver, SDPath) == 0)
  {
    /* Register the file system object to the FatFs module */
    if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
    {
      /* FatFs Initialization Error */
      Error_Handler();
    }
		/***************************WARNING*****************************
		// This function needs to be uncommented when using a new SD Card, 
		// but will delete all content on the Card if exectuted
    // Create a FAT file system (format) on the logical drive
    // WARNING: Formatting the uSD card will delete all content on the device 
    if(f_mkfs((TCHAR const*)SDPath, 0, 0) != FR_OK)
    {
			// FatFs Format Error 
      Error_Handler();
    }
		***************************************************************/
	}
}
void writeFile(User *user, char *fileName)
{
	FRESULT res;                            // FatFs function common result code
  uint32_t byteswritten;					        // File write/read counts
//  uint8_t *wtext = (uint8_t*)text;        // File write buffer
//	struct User *wtext = usr;
	
  /* Create and Open a new text file object with write access */
  if(f_open(&MyFile, (char*)fileName, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
  {
		// File Open Error
		Error_Handler();
  }
	
  else
  {
		/* Write data to the text file */
    res = f_write(&MyFile, user, sizeof(*user), (void *)&byteswritten);
          
    if((byteswritten == 0) || (res != FR_OK))
    {
			/* 'STM32.TXT' file Write or EOF Error */
			Error_Handler();
    }
    else
    {
			/* Close the open text file */
      f_close(&MyFile);
		}
	}
}

User* readFile(char *fileName)
{
	FRESULT res;                            // FatFs function common result code
  uint32_t bytesread;       							// File write/read counts
//  User* rtext; 										    // File read buffer
	User *output;
	
	/* Open the text file object with read access */
  if(f_open(&MyFile, (char*) fileName, FA_READ) != FR_OK)
  {
		// File Open Error
		//return (uint8_t*)'X';
		output = NULL;
  }
	
  else
  {
		/* Read data from the text file */
		res = f_read(&MyFile, output, sizeof(*output), (UINT*)&bytesread);
		
		if(res != FR_OK)
    {
			// 'STM32.TXT' file Read or EOF Error 
			Error_Handler();
    }
  }
	
	// Close the open text file 
	f_close(&MyFile);
	
  //output = (uint8_t*)rtext;
	
	return output;
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED1 on */
  BSP_LED_On(LED1);
  while(1)
  {
    BSP_LED_Toggle(LED1);
    HAL_Delay(200);
  }
}
