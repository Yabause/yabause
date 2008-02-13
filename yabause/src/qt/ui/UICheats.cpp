#include "UICheats.h"
#include "UICheatAR.h"
#include "UICheatRaw.h"
#include "../CommonDialogs.h"

UICheats::UICheats( QWidget* p )
	: QDialog( p )
{
	// set up dialog
	setupUi( this );
	// cheat counts
	int cheatsCount;
	// get cheats
	mCheats = CheatGetList( &cheatsCount );
	// add know cheats to treewidget
	for ( int id = 0; id < cheatsCount; id++ )
		addCode( id );
}

void UICheats::addCode( int id )
{
	// generate caption
	QString s;
	switch ( mCheats[id].type )
	{
		case CHEATTYPE_ENABLE:
			s = QString( "Enable Code : %1 %2" ).arg( (int)mCheats[id].addr, 8, 16, QChar( '0' ) ).arg( (int)mCheats[id].val, 8, 16, QChar( '0' ) );
			break;
		case CHEATTYPE_BYTEWRITE:
			s = QString( "Byte Write : %1 %2" ).arg( (int)mCheats[id].addr, 8, 16, QChar( '0' ) ).arg( (int)mCheats[id].val, 2, 16, QChar( '0' ) );
			break;
		case CHEATTYPE_WORDWRITE:
			s = QString( "Word Write : %1 %2" ).arg( (int)mCheats[id].addr, 8, 16, QChar( '0' ) ).arg( (int)mCheats[id].val, 4, 16, QChar( '0' ) );
			break;
		case CHEATTYPE_LONGWRITE:
			s = QString( "Long Write : %1 %2" ).arg( (int)mCheats[id].addr, 8, 16, QChar( '0' ) ).arg( (int)mCheats[id].val, 8, 16, QChar( '0' ) );
			break;
		default:
			break;
	}
	// update item
	QTreeWidgetItem* it = new QTreeWidgetItem( twCheats );
	it->setText( 0, s );
	it->setText( 1, mCheats[id].desc );
	it->setText( 2, mCheats[id].enable ? tr( "Enabled" ) : tr( "Disabled" ) );
	// enable buttons
	pbClear->setEnabled( true );
}

void UICheats::addARCode( const QString& c, const QString& d )
{
	// need check in list if already is code
	// add code
	if ( CheatAddARCode( c.toAscii().constData() ) != 0 )
	{
		CommonDialogs::information( tr( "Unable to add code" ) );
		return;
	}
	// change the description
	int cheatsCount;
	CheatGetList( &cheatsCount );
	if ( CheatChangeDescriptionByIndex( cheatsCount -1, d.toAscii().data() ) != 0 )
		CommonDialogs::information( tr( "Unable to change description" ) );
	// add code in treewidget
	addCode( cheatsCount -1 );
}

void UICheats::addRawCode( int t, const QString& a, const QString& v, const QString& d )
{
	// need check in list if already is code
	bool b;
	quint32 u;
	// check address
	u = a.toUInt( &b, 16 );
	if ( !b )
	{
		CommonDialogs::information( tr( "Invalid Address" ) );
		return;
	}
	// check value
	u = v.toUInt( &b, 16 );
	if ( !b )
	{
		CommonDialogs::information( tr( "Invalid Value" ) );
		return;
	}
	// add value
	if ( CheatAddCode( t, a.toUInt(), v.toUInt() ) != 0 )
	{
		CommonDialogs::information( tr( "Unable to add code" ) );
		return;
	}
	// get cheats and cheats count
	int cheatsCount;
	CheatGetList( &cheatsCount );
	// change description
	if ( CheatChangeDescriptionByIndex( cheatsCount -1, d.toAscii().data() ) != 0 )
		CommonDialogs::information( tr( "Unable to change description" ) );
	// add code in treewidget
	addCode( cheatsCount -1 );
}

void UICheats::on_twCheats_itemSelectionChanged()
{ pbDelete->setEnabled( twCheats->selectedItems().count() ); }

void UICheats::on_twCheats_itemDoubleClicked( QTreeWidgetItem* it, int )
{
	if ( it )
	{
		// get id of item
		int id = twCheats->indexOfTopLevelItem( it );
		// if ok
		if ( id != -1 )
		{
			// disable cheat
			if ( mCheats[id].enable )
				CheatDisableCode( id );
			// enable cheat
			else
				CheatEnableCode( id );
			// update treewidget item
			it->setText( 2, mCheats[id].enable ? tr( "Enabled" ) : tr( "Disabled" ) );
		}
	}
}

void UICheats::on_pbDelete_clicked()
{
	// get current selected item
	if ( QTreeWidgetItem* it = twCheats->selectedItems().value( 0 ) )
	{
		// get item id
		int id = twCheats->indexOfTopLevelItem( it );
		// remove cheat
		if ( CheatRemoveCodeByIndex( id ) != 0 )
		{
			CommonDialogs::information( tr( "Unable to remove code" ) );
			return;
		}
		// delete item
		delete it;
	}
}

void UICheats::on_pbClear_clicked()
{
	// clear cheats
	CheatClearCodes();
	// clear treewidget items
	twCheats->clear();
	// disable buttons
	pbDelete->setEnabled( false );
	pbClear->setEnabled( false );
}

void UICheats::on_pbAR_clicked()
{
	// add AR code if dialog exec
	UICheatAR d( this );
	if ( d.exec() )
		addARCode( d.leCode->text(), d.teDescription->toPlainText() );
}

void UICheats::on_pbRaw_clicked()
{
	// add RAW code if dialog exec
	UICheatRaw d( this );
	if ( d.exec() && d.type() != -1 )
		addRawCode( d.type(), d.leAddress->text(), d.leValue->text(), d.teDescription->toPlainText() );
}

void UICheats::on_pbFile_clicked()
{}
