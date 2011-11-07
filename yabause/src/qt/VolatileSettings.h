#ifndef VOLATILESETTINGS_H
#define VOLATILESETTINGS_H

#include <QString>
#include <QVariant>
#include <QMap>

class VolatileSettings
{
protected:
	QMap<QString, QVariant> mValues;

public:
	void setValue(const QString & key, const QVariant & value);
	QVariant value(const QString & key, const QVariant & defaultValue = QVariant()) const;
};

#endif
