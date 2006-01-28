/*
 *  cd.h
 *  Yabausemac
 *
 *  Created by Jerome Vernet on 24/01/06.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

int MacOSXCDInit(const char *);
int MacOSXCDDeInit();
int MacOSXCDGetStatus();
long MacOSXCDReadTOC(unsigned long *);
int MacOSXCDReadSectorFAD(unsigned long, void *);
