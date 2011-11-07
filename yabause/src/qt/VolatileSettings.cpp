#include "VolatileSettings.h"
#include "Settings.h"
#include "QtYabause.h"

#include <iostream>

void VolatileSettings::setValue(const QString & key, const QVariant & value)
{
	mValues[key] = value;
}

QVariant VolatileSettings::value(const QString & key, const QVariant & defaultValue) const
{
	if (mValues.contains(key))
		return mValues[key];

	Settings * s = QtYabause::settings();
	return s->value(key, defaultValue);
}
