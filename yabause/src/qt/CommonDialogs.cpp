#include "CommonDialogs.h"

bool CommonDialogs::question( const QString& m, const QString& c )
{ return QMessageBox::question( QApplication::activeWindow(), c, m, QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::Yes; }

void CommonDialogs::warning( const QString& m, const QString& c )
{ QMessageBox::warning( QApplication::activeWindow(), c, m ); }

void CommonDialogs::information( const QString& m, const QString& c )
{ QMessageBox::information( QApplication::activeWindow(), c, m ); }

QString CommonDialogs::getItem( const QStringList i, const QString& l, const QString& c )
{
	bool b;
	const QString s = QInputDialog::getItem( QApplication::activeWindow(), c, l, i, 0, false, &b );
	return b ? s : QString();
}

QString CommonDialogs::getSaveFileName( const QString& d, const QString& c, const QString& f )
{ return QFileDialog::getSaveFileName( QApplication::activeWindow(), c, d, f ); }

QString CommonDialogs::getOpenFileName( const QString& d, const QString& c, const QString& f )
{ return QFileDialog::getOpenFileName( QApplication::activeWindow(), c, d, f ); }

QString CommonDialogs::getExistingDirectory( const QString& d, const QString& c, QFileDialog::Options o )
{ return QFileDialog::getExistingDirectory( QApplication::activeWindow(), c, d, o ); }
