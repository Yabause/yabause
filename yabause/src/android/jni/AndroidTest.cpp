/**********************************
 Java Native Interface library
**********************************/
#include <jni.h>

/** This is the C++ implementation of the Java native method.
@param env Pointer to JVM environment
@param clazz Class containing native methods
*/
extern "C"
JNIEXPORT void JNICALL
Java_org_uoyabause_android_AndroidTest_androidtestNative( JNIEnv* env, jclass clazz )
{
	// Enter code here
}
