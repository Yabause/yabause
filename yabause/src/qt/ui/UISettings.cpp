#include "UISettings.h"
#include "QtYabause.h"

extern M68K_struct* M68KCoreList[];
extern SH2Interface_struct* SH2CoreList[];
extern PerInterface_struct* PERCoreList[];
extern CDInterface* CDCoreList[];
extern SoundInterface_struct* SNDCoreList[];
extern VideoInterface_struct* VIDCoreList[];

UISettings::UISettings( QWidget* p )
	: QDialog( p )
{
	// setup dialog
	setupUi( this );

	// load cores informations
	loadCores();

	// load settings
	loadSettings();
}

UISettings::~UISettings()
{}

void UISettings::loadCores()
{
	// CDInterface
	for ( int i = 0; CDCoreList[i] != NULL; i++ )
		cbCdRom->addItem( CDCoreList[i]->Name, CDCoreList[i]->id );
	
	// VDIInterface
	for ( int i = 0; VIDCoreList[i] != NULL; i++ )
		cbVideoCore->addItem( VIDCoreList[i]->Name, VIDCoreList[i]->id );
	
	// VDIFormatInterface
	/*
	for ( int i = 0; VIDCoreList[i] != NULL; i++ )
		cbVideoFormat->addItem( VIDCoreList[i]->Name, VIDCoreList[i]->id );
	*/
	
	// SNDInterface
	for ( int i = 0; SNDCoreList[i] != NULL; i++ )
		cbSoundCore->addItem( SNDCoreList[i]->Name, SNDCoreList[i]->id );
	
	// Cartridge
	//cbCartridge
	
	// Region
	//cbRegion
	
	// Interpreters
	for ( int i = 0; SH2CoreList[i] != NULL; i++ )
		cbSH2Interpreter->addItem( SH2CoreList[i]->Name, SH2CoreList[i]->id );
}

void UISettings::loadSettings()
{
	// get settings pointer
	Settings* s = QtYabause::settings();

	// general
	leBios->setText( s->value( "General/Bios" ).toString() );
	cbCdRom->setCurrentIndex( cbCdRom->findData( s->value( "General/CdRom" ).toInt() ) );
	leCdRom->setText( s->value( "General/CdRomISO" ).toString() );
	leSaveStates->setText( s->value( "General/SaveStates" ).toString() );

	// video
	cbVideoCore->setCurrentIndex( cbVideoCore->findData( s->value( "Video/VideoCore" ).toInt() ) );
	leWidth->setText( s->value( "Video/Width" ).toString() );
	leHeight->setText( s->value( "Video/Height" ).toString() );
	cbFullscreen->setChecked( s->value( "Video/Fullscreen" ).toBool() );
	cbVideoFormat->setCurrentIndex( cbVideoFormat->findData( s->value( "Video/VideoFormat" ).toInt() ) );

	// sound
	cbSoundCore->setCurrentIndex( cbSoundCore->findData( s->value( "Sound/SoundCore" ).toInt() ) );

	// cartridge/memory
	/*
	cbCartridge
	leCartridge->
	leMemory->
	leMpegROM->
	*/
	
	// input
	
	// advanced
	cbRegion->setCurrentIndex( cbRegion->findData( s->value( "Advanced/Region" ).toString() ) );
	cbSH2Interpreter->setCurrentIndex( cbSH2Interpreter->findData( s->value( "Advanced/SH2Interpreter" ).toInt() ) );
}

void UISettings::saveSettings()
{
	// get settings pointer
	Settings* s = QtYabause::settings();

	// general
	s->setValue( "General/Bios", leBios->text() );
	s->setValue( "General/CdRom", cbCdRom->itemData( cbCdRom->currentIndex() ).toInt() );
	s->setValue( "General/CdRomISO", leCdRom->text() );
	s->setValue( "General/SaveStates", leSaveStates->text() );

	// video
	s->setValue( "Video/VideoCore", cbVideoCore->itemData( cbVideoCore->currentIndex() ).toInt() );
	s->setValue( "Video/Width", leWidth->text() );
	s->setValue( "Video/Height", leHeight->text() );
	s->setValue( "Video/Fullscreen", cbFullscreen->isChecked() );
	s->setValue( "Video/VideoFormat", cbVideoFormat->itemData( cbVideoFormat->currentIndex() ).toInt() );

	// sound
	s->setValue( "Sound/SoundCore", cbSoundCore->itemData( cbSoundCore->currentIndex() ).toInt() );

	// cartridge/memory
	/*
	cbCartridge
	leCartridge->
	leMemory->
	leMpegROM->
	*/
	
	// input
	
	// advanced
	s->setValue( "Advanced/Region", cbRegion->itemData( cbRegion->currentIndex() ).toString() );
	s->setValue( "Advanced/SH2Interpreter", cbSH2Interpreter->itemData( cbSH2Interpreter->currentIndex() ).toInt() );
}

void UISettings::accept()
{
	saveSettings();
	QDialog::accept();
}
