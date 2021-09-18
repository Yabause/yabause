/*	Copyright 2012-2013 Theo Berkau <cwx@cyberwarriorx.com>

	This file is part of Yabause.

	Yabause is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Yabause is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Yabause; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#ifndef UIDEBUGSH2_H
#define UIDEBUGSH2_H

#include "UIDebugCPU.h"
#include "../QtYabause.h"

class UIDebugSH2 : public UIDebugCPU
{
	Q_OBJECT
private:
   SH2_struct *debugSH2;
   QString addr2line;
   void restoreAddr2line();

public:
   UIDebugSH2( UIDebugCPU::PROCTYPE proc, YabauseThread *mYabauseThread, QWidget* parent = 0 );
   void updateRegList();
   void updateCodeList(u32 addr);
   void updateCodePage(u32 evaluateAddress) override;
   void updateBackTrace();
   void updateTrackInfLoop();
   void updateAll();
   u32 getRegister(int index, int *size);
   void setRegister(int index, u32 value);
   bool addCodeBreakpoint(u32 addr);
   bool delCodeBreakpoint(u32 addr);
   bool addMemoryBreakpoint(u32 addr, u32 flags);
   bool delMemoryBreakpoint(u32 addr);
   void stepInto();
   void stepOver();
   void stepOut();
   void reserved1();
   void reserved2();
	void reserved3();
protected:

protected slots:
   void loadCodeAddress();
};

#endif // UIDEBUGSH2_H
