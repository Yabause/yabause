#ifndef CD_H
#define CD_H

int CDInit(char *cdrom_name);
int CDDeInit();
int CDGetStatus();
long CDReadToc(unsigned long *TOC);
bool CDReadSectorFAD(unsigned long FAD, void *buffer);

#endif
