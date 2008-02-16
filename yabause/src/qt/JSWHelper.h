#ifndef JSWHELPER_H
#define JSWHELPER_H

#include <QtGlobal>

extern "C"
{
	#include "3rdparty/include/jsw.h"
}

class JSWHelperJoystick
{
public:
	JSWHelperJoystick( js_attribute_struct* joystick = 0, const char* calibrationFile = 0 )
	{
		mJoystick = joystick;
		mJoystickDatas = new js_data_struct;
		if ( !mJoystick )
			mStatus = JSError;
		else
			mStatus = JSInit( mJoystickDatas, joystick->device_name, calibrationFile ? calibrationFile : JSDefaultCalibration, JSFlagNonBlocking );
	}

	~JSWHelperJoystick()
	{
		if ( mJoystick )
			JSClose( mJoystickDatas );
		delete mJoystickDatas;
	}

	int status() const
	{ return mStatus; }

	js_data_struct* datas() const
	{ return mJoystickDatas; }

protected:
	int k;
	int mStatus;
	js_attribute_struct* mJoystick;
	js_data_struct* mJoystickDatas;
};

class JSWHelperAttributes
{
public:
	JSWHelperAttributes( const char* cf = 0 )
	{
		mCalibrationFile = cf ? cf : JSDefaultCalibration;
		mAttributes = JSGetAttributesList( &mAttributesCount, mCalibrationFile );
	}

	~JSWHelperAttributes()
	{ JSFreeAttributesList( mAttributes, mAttributesCount ); }

	bool isValid() const
	{ return mAttributes; }

	js_attribute_struct* attributes() const
	{ return mAttributes; }

	int attributesCount() const
	{ return mAttributesCount; }

	const char* calibrationFile() const
	{ return mCalibrationFile; }

	JSWHelperJoystick* joystick( int i ) const
	{ return mAttributes ? new JSWHelperJoystick( &mAttributes[i], mCalibrationFile ) : 0; }

protected:
	js_attribute_struct* mAttributes;
	int mAttributesCount;
	const char* mCalibrationFile;
};

namespace JSWHelper
{
	JSWHelperAttributes* joysticks();
	const char* errorString( int status );
};

#endif // JSWHELPER_H
