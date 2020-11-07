package org.uoyabause.android

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
object AndroidTest {
    /**
     * An example native method.  See the library function,
     * `Java_org_uoyabause_android_AndroidTest_androidtestNative`
     * for the implementation.
     */
    external fun androidtestNative()

    /* This is the static constructor used to load the
	 * 'AndroidTest' library when the class is
	 * loaded.
	 */
    init {
        System.loadLibrary("AndroidTest")
    }
}