#include "UICheatRaw.h"
#include "../QtYabause.h"

#include <QButtonGroup>

UICheatRaw::UICheatRaw( QWidget* p )
	: QDialog( p )
{
	// setup dialog
	setupUi( this );

	// fill types
	mButtonGroup = new QButtonGroup( this );
	mButtonGroup->addButton( rbEnable, CHEATTYPE_ENABLE );
	mButtonGroup->addButton( rbByte, CHEATTYPE_BYTEWRITE );
	mButtonGroup->addButton( rbWord, CHEATTYPE_WORDWRITE );
	mButtonGroup->addButton( rbLong, CHEATTYPE_LONGWRITE );
}
