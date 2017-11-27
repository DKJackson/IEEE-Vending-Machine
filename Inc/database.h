/* Define to prevent recursive inclusion */
#ifndef __DATABASE_H
#define __DATABASE_H

void databaseInit(void);
void writeUser(struct User *user, char *fileName);
void writeItem(struct Item *item, char *fileName);
User* readUser(char *fileName);
Item* readItem(char *fileName);

#endif 
