#ifndef CD_H
#define CD_H

int CDInit(char *cdrom_name);
int CDDeInit();
bool CDIsCDPresent();
int CDGetStatus();
long CDReadToc(unsigned long *TOC);
unsigned long CDReadSector(unsigned long lba, unsigned long size, void *buffer);
bool CDReadSectorFAD(unsigned long FAD, void *buffer);

#endif
