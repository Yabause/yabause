#include "UIAbout.h"

UIAbout::UIAbout( QWidget* o )
	: QDialog( o )
{
	setupUi( this );
	lInformations->setText( lInformations->text().arg( VERSION ) );
}
