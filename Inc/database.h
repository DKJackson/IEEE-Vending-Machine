/* Define to prevent recursive inclusion */
#ifndef __DATABASE_H
#define __DATABASE_H

void databaseInit(void);
void writeFile(User *user, char *fileName);
struct User* readFile(char *fileName);

#endif 
