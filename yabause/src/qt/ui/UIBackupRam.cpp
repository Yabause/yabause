#include "UIBackupRam.h"
#include "../CommonDialogs.h"
#include "QtYabause.h"

u32 currentbupdevice = 0;
deviceinfo_struct* devices = NULL;
int numbupdevices = 0;
saveinfo_struct* saves = NULL;
int numsaves = 0;

UIBackupRam::UIBackupRam( QWidget* p )
	: QDialog( p )
{
	//setup dialog
	setupUi( this );
	if ( p && !p->isFullScreen() )
		setWindowFlags( Qt::Sheet );

	// get available devices
	if ( ( devices = BupGetDeviceList( &numbupdevices ) ) == NULL )
		return;
	
	// add to combobox
	for ( int i = 0; i < numbupdevices; i++ )
		cbDeviceList->addItem( devices[i].name, devices[i].id );
	
	// get save list for current devices
	refreshSaveList();
}

void UIBackupRam::refreshSaveList()
{
	// blocks
	u32 fs = 0, ms = 0;
	u32 id = cbDeviceList->itemData( cbDeviceList->currentIndex() ).toInt();

	// clear listwidget
	lwSaveList->clear();

	// get save list
	saves = BupGetSaveList( id, &numsaves);

	// add item to listwidget
	for ( int i = 0; i < numsaves; i++ )
		lwSaveList->addItem( saves[i].filename );
	
	// set infos about blocks
	BupGetStats( id, &fs, &ms );
	lBlocks->setText( tr( "%1/%2 blocks free" ).arg( fs ).arg( ms ) );
	
	// enable/disable button delete according to available item
	pbDelete->setEnabled( lwSaveList->count() );
	
	// select first item in the item list
	if ( lwSaveList->count() )
		lwSaveList->setCurrentRow( 0 );
	on_lwSaveList_itemSelectionChanged();
}

void UIBackupRam::on_cbDeviceList_currentIndexChanged( int )
{ refreshSaveList(); }

void UIBackupRam::on_lwSaveList_itemSelectionChanged()
{
	// get current save id
	int id = lwSaveList->currentRow();

	// update gui
	if ( id != -1 )
	{
		leFileName->setText( saves[id].filename );
		leComment->setText( saves[id].comment );
		switch ( saves[id].language )
		{
			case 0:
				leLanguage->setText( tr( "Japanese" ) );
				break;
			case 1:
				leLanguage->setText( tr( "English" ) );
				break;
			case 2:
				leLanguage->setText( tr( "French" ) );
				break;
			case 3:
				leLanguage->setText( tr( "German" ) );
				break;
			case 4:
				leLanguage->setText( tr( "Spanish" ) );
				break;
			case 5:
				leLanguage->setText( tr( "Italian" ) );
				break;
			default:
				leLanguage->setText( tr( "Unknow (%1)" ).arg( saves[id].language ) );
				break;
		}
		leDataSize->setText( QString::number( saves[id].datasize ) );
		leBlockSize->setText( QString::number( saves[id].blocksize ) );
	}
	else
	{
		// clear gui
		leFileName->clear();
		leComment->clear();
		leLanguage->clear();
		leDataSize->clear();
		leBlockSize->clear();
	}
}

void UIBackupRam::on_pbDelete_clicked()
{
	if ( QListWidgetItem* it = lwSaveList->selectedItems().value( 0 ) )
	{
		u32 id = cbDeviceList->itemData( cbDeviceList->currentIndex() ).toInt();
		if ( CommonDialogs::question( tr( "Are you sure you want to delete '%1' ?" ).arg( it->text() ) ) )
		{
			BupDeleteSave( id, it->text().toAscii().constData() );
			refreshSaveList();
		}
	}
}

void UIBackupRam::on_pbFormat_clicked()
{
	u32 id = cbDeviceList->itemData( cbDeviceList->currentIndex() ).toInt();
	if ( CommonDialogs::question( tr( "Are you sure you want to format '%1' ?" ).arg( devices[id].name ) ) )
	{
		BupFormat( id );
		refreshSaveList();
	}
}
