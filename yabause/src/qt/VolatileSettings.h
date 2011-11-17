#ifndef VOLATILESETTINGS_H
#define VOLATILESETTINGS_H

#include <QString>
#include <QVariant>
#include <QHash>

class VolatileSettings
{
protected:
	QHash<QString, QVariant> mValues;

public:
	void clear();
	void setValue(const QString & key, const QVariant & value);
	void removeValue(const QString& key);
	QVariant value(const QString & key, const QVariant & defaultValue = QVariant()) const;
};

#endif
