/*  Copyright 2005 Guillaume Duhamel
	Copyright 2005-2006 Theo Berkau
	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>

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
#ifndef QTYABAUSE_H
#define QTYABAUSE_H

extern "C"
{
	#include "../yabause.h"
	#include "../peripheral.h"
	#include "../sh2core.h"
	#include "../sh2int.h"
	#include "../vidogl.h"
	#include "../vidsoft.h"
	#include "../cs0.h"
	#include "../cdbase.h"
	#include "../scsp.h"
	#include "../sndal.h"
	#include "../sndsdl.h"
	#include "../persdljoy.h"
	#include "../permacjoy.h"
	#include "../debug.h"
	#include "../m68kcore.h"
	#include "../m68kc68k.h"

	#include "../vdp1.h"
	#include "../vdp2.h"
	#include "../cs2.h"

	#include "../cheat.h"
	#include "../memory.h"
	#include "../bios.h"
	
	#include "PerQt.h"
#ifdef Q_OS_WIN
	#include "../windows/cd.h"
#endif
}

#include <QString>
#include <QMap>

class UIYabause;
class Settings;
class QWidget;

namespace QtYabause
{
	UIYabause* mainWindow();
	Settings* settings();
	int setTranslationFile();
	int logTranslation();
	void closeTranslation();
	QString translate( const QString& string );
	void retranslateWidget( QWidget* widget );
	void retranslateApplication();

	// get cd serial
	const char* getCurrentCdSerial();

	// get core by id
	M68K_struct* getM68KCore( int id );
	SH2Interface_struct* getSH2Core( int id );
	PerInterface_struct* getPERCore( int id );
	CDInterface* getCDCore( int id );
	SoundInterface_struct* getSNDCore( int id );
	VideoInterface_struct* getVDICore( int id );

	// default cores
	CDInterface defaultCDCore();
	SoundInterface_struct defaultSNDCore();
	VideoInterface_struct defaultVIDCore();
	PerInterface_struct defaultPERCore();
	SH2Interface_struct defaultSH2Core();
	
	// padsbits
	QMap<uint, PerPad_struct*>* portPadsBits( uint portNumber );
	void clearPadsBits();
};

#endif // QTYABAUSE_H
