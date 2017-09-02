/* Define to prevent recursive inclusion */
#ifndef __DATABASE_H
#define __DATABASE_H

void databaseInit(void);
void writeFile(uint8_t *text, char *fileName);
uint8_t* readFile(char *fileName);

#endif 
