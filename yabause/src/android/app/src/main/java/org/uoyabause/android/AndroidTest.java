package org.uoyabause.android;

/**
 * This class loads the Java Native Interface (JNI)
 * library, 'libAndroidTest.so', and provides access to the
 * exposed C functions.
 * The library is packaged and installed with the application.
 * See the C file, /jni/AndroidTest.c file for the
 * implementations of the native methods. 
 * 
 * For more information on JNI, see: http://java.sun.com/docs/books/jni/
 */
public class AndroidTest
{ 
	/**
	 * An example native method.  See the library function,
	 * <code>Java_org_uoyabause_android_AndroidTest_androidtestNative</code>
	 * for the implementation.
	 */
	static public native void androidtestNative();

	/* This is the static constructor used to load the
	 * 'AndroidTest' library when the class is
	 * loaded.
	 */
	static {
		System.loadLibrary("AndroidTest");
	}
}
