#include "UICheatAR.h"
#include "../QtYabause.h"

UICheatAR::UICheatAR( QWidget* p )
	: QDialog( p )
{
	// setup dialog
	setupUi( this );
	// retranslate widgets
	QtYabause::retranslateWidget( this );
}
