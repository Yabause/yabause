/*	Copyright 2013 Theo Berkau <cwx@cyberwarriorx.com>

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
#ifndef UISHORTCUTMANAGER_H
#define UISHORTCUTMANAGER_H

#include <QTableWidget.h>
#include <QStyledItemDelegate.h>
#include <QEvent.h>
#include <QLineEdit.h>
#include "../QtYabause.h"

class MyItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	MyItemDelegate(QObject *parent = 0) : QStyledItemDelegate(parent)
	{
		installEventFilter(this);
	}
	bool eventFilter ( QObject * editor, QEvent * event )
	{
		QEvent::Type type = event->type();
		if (type == QEvent::KeyRelease)
		{
			//((QLineEdit *)editor)->text();

		}
		else if (type == QEvent::KeyPress)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

			int key = keyEvent->key();

			if(key == Qt::Key_unknown){ 
				//qDebug() << "Unknown key from a macro probably"; 
				//return; 
				return true;
			} 
			QString text;

			if (keyEvent->key() == Qt::Key_Shift ||
				keyEvent->key() == Qt::Key_Control ||
				keyEvent->key() == Qt::Key_Meta ||
				keyEvent->key() == Qt::Key_Alt)
				text = QKeySequence(keyEvent->modifiers()).toString();
			else
			   text = QKeySequence(keyEvent->modifiers() + keyEvent->key()).toString();
			((QLineEdit *)editor)->setText(text);
			return true;
		}
		return QStyledItemDelegate::eventFilter(editor, event);
	}
};

class UIShortcutManager : public QTableWidget
{
	Q_OBJECT

public:
	UIShortcutManager( QWidget* parent = 0 );
	virtual ~UIShortcutManager();


protected:

protected slots:
};

#endif // UISHORTCUTMANAGER_H
