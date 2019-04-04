#define EXTENSION_NAME GpgExt
#define LIB_NAME "GpgExt"
#define MODULE_NAME "gpg"

#define DLIB_LOG_DOMAIN LIB_NAME

#if defined(DM_PLATFORM_ANDROID)

#include "gpgs_jni.h"

struct AuthState
{
    jobject                 m_GpgsJNI;
    jmethodID               m_silentLogin;
};

AuthState g_Auth;

int GpgAuth_Login(lua_State* L)
{
    return 0;
}

int GpgAuth_Logout(lua_State* L)
{
    return 0;
}

int GpgAuth_SilentLogin(lua_State* L)
{
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    env->CallVoidMethod(g_Auth.m_GpgsJNI, g_Auth.m_silentLogin);
    return 0;
}

void GpgAuth_Init()
{
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    ClassLoader class_loader = ClassLoader(env);
    jclass cls = class_loader.load("com.defold.gpgs.GpgsJNI");

    g_Auth.m_silentLogin = env->GetMethodID(cls, "silentLogin", "()V");
    
    jmethodID jni_constructor = env->GetMethodID(cls, "<init>", "(Landroid/app/Activity;)V");
    g_Auth.m_GpgsJNI = env->NewGlobalRef(env->NewObject(cls, jni_constructor, dmGraphics::GetNativeAndroidActivity()));
}

#endif