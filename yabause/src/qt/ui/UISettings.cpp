#include "UISettings.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QList>
#include <QTimer>
#include <QTime>

extern M68K_struct* M68KCoreList[];
extern SH2Interface_struct* SH2CoreList[];
extern PerInterface_struct* PERCoreList[];
extern CDInterface* CDCoreList[];
extern SoundInterface_struct* SNDCoreList[];
extern VideoInterface_struct* VIDCoreList[];

struct Item
{
	Item( const QString& k, const QString& s )
	{ id = k; Name = s; }
	
	QString id;
	QString Name;
};

typedef QList<Item> Items;

const Items mRegions = Items()
	<< Item( "Auto" , "Auto-detect" )
	<< Item( "J" , "Japan" )
	<< Item( "T", "Asia" )
	<< Item( "U", "North America" )
	<< Item( "B", "Central/South America" )
	<< Item( "K", "Korea" )
	<< Item( "A", "Asia" )
	<< Item( "E", "Europe + others" )
	<< Item( "L", "Central/South America" );

const Items mCartridgeTypes = Items()
	<< Item( "0", "None" )
	<< Item( "1", "Pro Action Replay" )
	<< Item( "2", "4 Mbit Backup Ram" )
	<< Item( "3", "8 Mbit Backup Ram" )
	<< Item( "4", "16 Mbit Backup Ram" )
	<< Item( "5", "32 Mbit Backup Ram" )
	<< Item( "6", "8 Mbit Dram" )
	<< Item( "7", "32 Mbit Dram" )
	<< Item( "8", "Netlink" )
	<< Item( "9", "16 Mbit ROM" );

const Items mVideoFromats = Items()
	<< Item( "0", "NTSC" )
	<< Item( "1", "PAL" );

const int TIMEOUT_SCAN = 1000;

UISettings::UISettings( QWidget* p )
	: QDialog( p )
{
	mScanningInput = false;
	
	// setup dialog
	setupUi( this );

	// load cores informations
	loadCores();

	// load settings
	loadSettings();
	
	// create timer for input scan
	QTimer* mTimerInputScan = new QTimer( this );
	mTimerInputScan->setInterval( TIMEOUT_SCAN / 2 );
	
	// connections
	foreach ( QToolButton* tb, findChildren<QToolButton*>() )
		connect( tb, SIGNAL( clicked() ), this, SLOT( tbBrowse_clicked() ) );
	connect( mTimerInputScan, SIGNAL( timeout() ), this, SLOT( inputScan_timeout() ) );
	
	// start input scan
	mTimerInputScan->start();
}

UISettings::~UISettings()
{}

void UISettings::qSleep( int ms )
{
    Q_ASSERT( ms > 0 );
#ifdef Q_OS_WIN
    Sleep( uint( ms ) );
#else
    struct timespec ts = { ms /1000, ( ms %1000 ) *1000 *1000 };
    nanosleep( &ts, NULL );
#endif
}

void UISettings::requestFile( const QString& c, QLineEdit* e )
{
	const QString s = QFileDialog::getOpenFileName( window(), c, e->text() );
	if ( !s.isNull() )
		e->setText( s );
}

void UISettings::requestFolder( const QString& c, QLineEdit* e )
{
	const QString s = QFileDialog::getExistingDirectory( window(), c, e->text() );
	if ( !s.isNull() )
		e->setText( s );
}

void UISettings::requestDrive( const QString& c, QLineEdit* e )
{
	// get all drives
	QStringList drives;
	foreach ( const QFileInfo& fi, QDir::drives() )
		drives << fi.fileName();
	
	bool b;
	const QString s = QInputDialog::getItem( window(), tr( "CD Rom Drive..." ), c, drives, 0, false, &b );
	if ( b && !s.isNull() )
		e->setText( s );
}

void UISettings::inputScan_timeout()
{
	// cancel if already listening input
	if ( mScanningInput )
		return;
	
	// get focused lineedit
	QLineEdit* le = qobject_cast<QLineEdit*>( QApplication::focusWidget() );
	if ( !le || le->parent() != wInput )
		return;
	
	// get per core id
	int i = PERCORE_DUMMY;
#ifdef HAVE_LIBSDL && USENEWPERINTERFACE
	i = PERCORE_SDLJOY;
#endif
	
	// get percore pointer
	PerInterface_struct* c = QtYabause::getPERCore( i );
	
	// check if we have valid core & can scan keys
	if ( !c || c->canScan != 1 )
		return;
	
	// tell we are listening input
	mScanningInput = true;
	
	// get pressed input for key stock in the lineedit->statusTip()
	const char* ki = strdup( le->statusTip().toAscii().constData() );
	u32 k = 0;
	QTime t;
	t.start();
	do
	{
		c->Flush();
		k = c->Scan( ki );
		QCoreApplication::processEvents( QEventLoop::AllEvents, TIMEOUT_SCAN );
		qSleep( 10 );
	} while ( t.elapsed() < TIMEOUT_SCAN && k == 0 && QApplication::focusWidget() == le );
	
	// write it in the settings dialog
	if ( k != 0 )
		le->setText( QString::number( k ) );
	
	// tell we no longer listening input
	mScanningInput = false;
}

void UISettings::tbBrowse_clicked()
{
	// get toolbutton sender
	QToolButton* tb = qobject_cast<QToolButton*>( sender() );
	
	if ( tb == tbBios )
		requestFile( tr( "Choose a bios file" ), leBios );
	else if ( tb == tbCdRom )
	{
		if ( cbCdRom->currentText().contains( "dummy", Qt::CaseInsensitive ) )
		{
			QMessageBox::information( window(), tr( "Informations..." ), tr( "The dummies cores don't need configuration." ) );
			return;
		}
		else if ( cbCdRom->currentText().contains( "iso", Qt::CaseInsensitive ) )
			requestFile( tr( "" ), leCdRom );
		else
		{
#ifdef Q_OS_WIN
			requestDrive( tr( "Choose a cdrom drive" ), leCdRom );
#else
			requestFolder( tr( "Choose a cdrom mount point" ), leCdRom );
#endif
		}
	}
	else if ( tb == tbSaveStates )
		requestFolder( tr( "Choose a folder to store save states" ), leSaveStates );
	else if ( tb == tbCartridge )
	{
		QMessageBox::information( window(), tr( "Informations..." ), tr( "Not yet implemented" ) );
		return;
	}
	else if ( tb == tbMemory )
		requestFile( tr( "Choose a memory file" ), leMemory );
	else if ( tb == tbMpegROM )
		requestFile( tr( "Choose a mpeg rom" ), leMpegROM );
}

void UISettings::loadCores()
{
	// CD Drivers
	for ( int i = 0; CDCoreList[i] != NULL; i++ )
		cbCdRom->addItem( CDCoreList[i]->Name, CDCoreList[i]->id );
	
	// VDI Drivers
	for ( int i = 0; VIDCoreList[i] != NULL; i++ )
		cbVideoCore->addItem( VIDCoreList[i]->Name, VIDCoreList[i]->id );
	
	// Video Formats
	foreach ( const Item& it, mVideoFromats )
		cbVideoFormat->addItem( it.Name, it.id );
	
	// SND Drivers
	for ( int i = 0; SNDCoreList[i] != NULL; i++ )
		cbSoundCore->addItem( SNDCoreList[i]->Name, SNDCoreList[i]->id );
	
	// Cartridge Types
	foreach ( const Item& it, mCartridgeTypes )
		cbCartridge->addItem( it.Name, it.id );
	
	// Regions
	foreach ( const Item& it, mRegions )
		cbRegion->addItem( it.Name, it.id );
	
	// SH2 Interpreters
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
	cbCartridge->setCurrentIndex( cbCartridge->findData( s->value( "Cartridge/Type" ).toInt() ) );
	leCartridge->setText( s->value( "Cartridge/Path" ).toString() );
	leMemory->setText( s->value( "Memory/Path" ).toString() );
	leMpegROM->setText( s->value( "MpegROM/Path" ).toString() );
	
	// input
	foreach ( QLineEdit* le, wInput->findChildren<QLineEdit*>() )
		le->setText( s->value( QString( "Input/%1" ).arg( le->statusTip() ) ).toString() );
	
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
	s->setValue( "Cartridge/Type", cbCartridge->itemData( cbCartridge->currentIndex() ).toInt() );
	s->setValue( "Cartridge/Path", leCartridge->text() );
	s->setValue( "Memory/Path", leMemory->text() );
	s->setValue( "MpegROM/Path", leMpegROM->text() );
	
	// input
	foreach ( QLineEdit* le, wInput->findChildren<QLineEdit*>() )
		s->setValue( QString( "Input/%1" ).arg( le->statusTip() ), le->text() );
	
	// advanced
	s->setValue( "Advanced/Region", cbRegion->itemData( cbRegion->currentIndex() ).toString() );
	s->setValue( "Advanced/SH2Interpreter", cbSH2Interpreter->itemData( cbSH2Interpreter->currentIndex() ).toInt() );
}

void UISettings::accept()
{
	saveSettings();
	QDialog::accept();
}
