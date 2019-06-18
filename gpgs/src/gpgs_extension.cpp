#define EXTENSION_NAME GpgExt
#define LIB_NAME "GpgExt"
#define MODULE_NAME "gpg"

#define DLIB_LOG_DOMAIN LIB_NAME
#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)

#include "gpgs_jni.h"

struct GPGS
{
    jobject                 GpgsJNI;

    jmethodID               m_silentLogin;
    jmethodID               m_login;
    jmethodID               m_logout;
    jmethodID               m_activityResult;
    jmethodID               m_getDisplayName;
    jmethodID               m_getId;
    jmethodID               m_isAuthorized;
};

GPGS g_gpgs;

static void OnActivityResult(void *env, void* activity, int32_t request_code, int32_t result_code, void* result)
{
    dmLogInfo("OnActivityResult: env: %p  activity: %p  request_code: %d  result_code: %d  result: %p", env, activity, request_code, result_code, result);

    ThreadAttacher attacher;
    JNIEnv *_env = attacher.env;
    
    _env->CallVoidMethod(g_gpgs.GpgsJNI, g_gpgs.m_activityResult, request_code, result_code, result);
}

// GPGPS autorization 

int GpgAuth_Login(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    env->CallVoidMethod(g_gpgs.GpgsJNI, g_gpgs.m_login);
    
    return 0;
}

int GpgAuth_Logout(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    env->CallVoidMethod(g_gpgs.GpgsJNI, g_gpgs.m_logout);
    
    return 0;
}

int GpgAuth_SilentLogin(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    env->CallVoidMethod(g_gpgs.GpgsJNI, g_gpgs.m_silentLogin);
    
    return 0;
}

int GpgAuth_getDisplayName(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    jstring return_value = (jstring)env->CallObjectMethod(g_gpgs.GpgsJNI, g_gpgs.m_getDisplayName);
    if (return_value) 
    {
        const char* new_char = env->GetStringUTFChars(return_value, 0);
        env->DeleteLocalRef(return_value);
        lua_pushstring(L, new_char);
    }
    else
    {
        lua_pushnil(L);
    }
    
    return 1;
}

int GpgAuth_getId(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    jstring return_value = (jstring)env->CallObjectMethod(g_gpgs.GpgsJNI, g_gpgs.m_getId);
    if (return_value) 
    {
        const char* new_char = env->GetStringUTFChars(return_value, 0);
        env->DeleteLocalRef(return_value);
        lua_pushstring(L, new_char);
    }
    else
    {
        lua_pushnil(L);
    }
    
    return 1;
}

int GpgAuth_isAuthorized(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    jboolean return_value = (jboolean)env->CallBooleanMethod(g_gpgs.GpgsJNI, g_gpgs.m_isAuthorized);
    
    lua_pushboolean(L, JNI_TRUE == return_value);

    return 1;
}

void GpgAuth_Init(jclass cls, JNIEnv *env)
{
    g_gpgs.m_silentLogin = env->GetMethodID(cls, "silentLogin", "()V");
    g_gpgs.m_login = env->GetMethodID(cls, "login", "()V");
    g_gpgs.m_logout = env->GetMethodID(cls, "logout", "()V");
    g_gpgs.m_getDisplayName = env->GetMethodID(cls, "getDisplayName", "()Ljava/lang/String;");
    g_gpgs.m_getId = env->GetMethodID(cls, "getId", "()Ljava/lang/String;");
}

//

//gpg.methods()
static const luaL_reg Gpg_methods[] =
{
    {0,0}
};

//gpg.auth.methods()
static const luaL_reg Gpg_auth[] =
{
    {"login", GpgAuth_Login},
    {"logout", GpgAuth_Logout},
    {"silent_login", GpgAuth_SilentLogin},
    {"get_display_name", GpgAuth_getDisplayName},
    {"get_id", GpgAuth_getId},
    {"is_authorized", GpgAuth_isAuthorized},
    {0,0}
};

static dmExtension::Result AppInitializeGpg(dmExtension::AppParams* params)
{
    dmLogInfo("Registered extension Gpg");
    return dmExtension::RESULT_OK;
}

static void LuaInit(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    luaL_register(L, MODULE_NAME, Gpg_methods);

    lua_newtable(L);
    luaL_register(L, NULL, Gpg_auth);
    lua_setfield(L, -2, "auth");

    lua_pop(L,  1);
}

static void InitializeJNI()
{
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    ClassLoader class_loader = ClassLoader(env);
    jclass cls = class_loader.load("com.defold.gpgs.GpgsJNI");

    GpgAuth_Init(cls, env);

    g_gpgs.m_activityResult = env->GetMethodID(cls, "activityResult", "(IILandroid/content/Intent;)V");
    
    jmethodID jni_constructor = env->GetMethodID(cls, "<init>", "(Landroid/app/Activity;)V");
    g_gpgs.GpgsJNI = env->NewGlobalRef(env->NewObject(cls, jni_constructor, dmGraphics::GetNativeAndroidActivity()));
}

static dmExtension::Result InitializeGpg(dmExtension::Params* params)
{
    LuaInit(params->m_L);
    InitializeJNI();
    dmExtension::RegisterAndroidOnActivityResultListener(OnActivityResult);
    dmLogInfo("Initializing extension Gpg");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeGpg(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeGpg(dmExtension::Params* params)
{
    dmExtension::UnregisterAndroidOnActivityResultListener(OnActivityResult);
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, AppInitializeGpg, AppFinalizeGpg, InitializeGpg, 0, 0, FinalizeGpg)

#else

dmExtension::Result InitializeGpg(dmExtension::Params* params)
{
    dmLogInfo("Registered extension Gpg (null)");
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeGpg(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, 0, 0, InitializeGpg, 0, 0, FinalizeGpg)

#endif