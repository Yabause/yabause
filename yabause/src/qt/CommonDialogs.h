#ifndef COMMONDIALOGS_H
#define COMMONDIALOGS_H

#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>

namespace CommonDialogs
{
	bool question( const QString& message, const QString& caption = QT_TRANSLATE_NOOP( "CommonDialogs", "Question..." ) );
	void warning( const QString& message, const QString& caption = QT_TRANSLATE_NOOP( "CommonDialogs", "Warning..." ) );
	void information( const QString& message, const QString& caption = QT_TRANSLATE_NOOP( "CommonDialogs", "Information..." ) );
	QString getItem( const QStringList items, const QString& label, const QString& caption = QT_TRANSLATE_NOOP( "CommonDialogs", "Get Item..." ) );
	QString getSaveFileName( const QString& directory = QString(), const QString& filter = QString(), const QString& caption = QT_TRANSLATE_NOOP( "CommonDialogs", "Get Save File Name..." ) );
	QString getOpenFileName( const QString& directory = QString(), const QString& filter = QString(), const QString& caption = QT_TRANSLATE_NOOP( "CommonDialogs", "Get Open File Name..." ) );
	QString getExistingDirectory( const QString& directory = QString(), const QString& caption = QT_TRANSLATE_NOOP( "CommonDialogs", "Get Existing Directory..." ), QFileDialog::Options options = QFileDialog::ShowDirsOnly );
};

#endif // COMMONDIALOGS_H
