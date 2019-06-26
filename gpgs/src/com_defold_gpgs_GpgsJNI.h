#include <jni.h>
/* Header for class com_defold_gpgs_GpgsJNI */

#ifndef _Included_com_defold_gpgs_GpgsJNI
#define _Included_com_defold_gpgs_GpgsJNI
#ifdef __cplusplus
extern "C" {
	#endif
	/*
	* Class:     com_defold_gpgs_GpgsJNI
	* Method:    gpgsAddToQueue
	* Signature: (ILjava/lang/String;ILjava/lang/String;Ljava/lang/String;)V
	*/
	JNIEXPORT void JNICALL Java_com_defold_gpgs_GpgsJNI_gpgsAddToQueue
		(JNIEnv *, jclass, jint, jstring, jint, jstring, jstring);

	/*
	* Class:     com_defold_gpgs_GpgsJNI
	* Method:    gpgsAddToQueue_first_arg
	* Signature: (ILjava/lang/String;I)V
	*/
	JNIEXPORT void JNICALL Java_com_defold_gpgs_GpgsJNI_gpgsAddToQueueFirstArg
		(JNIEnv *, jclass, jint, jstring, jint);

	#ifdef __cplusplus
}
#endif
#endif