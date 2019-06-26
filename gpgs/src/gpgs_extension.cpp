#define EXTENSION_NAME GpgExt
#define LIB_NAME "GpgExt"
#define MODULE_NAME "gpg"

#define DLIB_LOG_DOMAIN LIB_NAME
#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)

#include "gpgs_jni.h"
#include "private_gpgs_callback.h"
#include "com_defold_gpgs_GpgsJNI.h"

struct GPGS
{
    jobject                 m_GpgsJNI;
    //autorization
    jmethodID               m_silentLogin;
    jmethodID               m_login;
    jmethodID               m_logout;
    jmethodID               m_activityResult;
    jmethodID               m_getDisplayName;
    jmethodID               m_getId;
    jmethodID               m_isAuthorized;
    jmethodID               m_setGravityForPopups;
};

GPGS g_gpgs;

// GPGPS autorization 

int GpgAuth_Login(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_login);
    
    return 0;
}

int GpgAuth_Logout(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_logout);
    
    return 0;
}

int GpgAuth_SilentLogin(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_silentLogin);
    
    return 0;
}

int GpgAuth_getDisplayName(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    jstring return_value = (jstring)env->CallObjectMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_getDisplayName);
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
    
    jstring return_value = (jstring)env->CallObjectMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_getId);
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
    
    jboolean return_value = (jboolean)env->CallBooleanMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_isAuthorized);
    
    lua_pushboolean(L, JNI_TRUE == return_value);

    return 1;
}

enum PopupPositions
{
    POPUP_POS_TOP_LEFT =           48 | 3,
    POPUP_POS_TOP_CENTER =         48 | 1,
    POPUP_POS_TOP_RIGHT =          48 | 5,
    POPUP_POS_CENTER_LEFT =        16 | 3,
    POPUP_POS_CENTER =             16 | 1,
    POPUP_POS_CENTER_RIGHT =       16 | 5,
    POPUP_POS_BOTTOM_LEFT =        80 | 3,
    POPUP_POS_BOTTOM_CENTER =      80 | 1,
    POPUP_POS_BOTTOM_RIGHT =       80 | 5
};

int GpgAuth_setGravityForPopups(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;

    int position_lua = luaL_checknumber(L, 1);

    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_setGravityForPopups, position_lua);

    return 0;
}

int Gpg_callback(lua_State* L)
{
    gpgs_set_callback(L, 1);
    return 0;
}

// Extention methods

static void OnActivityResult(void *env, void* activity, int32_t request_code, int32_t result_code, void* result)
{
    ThreadAttacher attacher;
    JNIEnv *_env = attacher.env;

    _env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_activityResult, request_code, result_code, result);
}

JNIEXPORT void JNICALL Java_com_defold_gpgs_GpgsJNI_gpgsAddToQueue(JNIEnv * env, jclass cls, jint jmsg, jstring jkey_1, jint jvalue_1, jstring jkey_2, jstring jvalue_2)
{
    const char* key_1 = env->GetStringUTFChars(jkey_1, 0);
    const char* key_2 = env->GetStringUTFChars(jkey_2, 0);
    const char* value_2 = env->GetStringUTFChars(jvalue_2, 0);
    gpgs_add_to_queue((int)jmsg, key_1, (int)jvalue_1, key_2, value_2);
    env->ReleaseStringUTFChars(jkey_1, key_1);
    env->ReleaseStringUTFChars(jkey_2, key_2);
    env->ReleaseStringUTFChars(jvalue_2, value_2);
}

JNIEXPORT void JNICALL Java_com_defold_gpgs_GpgsJNI_gpgsAddToQueueFirstArg(JNIEnv * env, jclass cls, jint jmsg, jstring jkey_1, jint jvalue_1)
{
    const char* key_1 = env->GetStringUTFChars(jkey_1, 0);
    gpgs_add_to_queue((int)jmsg, key_1, (int)jvalue_1, NULL, NULL);
    env->ReleaseStringUTFChars(jkey_1, key_1);
}
//-----

static const luaL_reg Gpg_methods[] =
{
    //autorization
    {"login", GpgAuth_Login},
    {"logout", GpgAuth_Logout},
    {"silent_login", GpgAuth_SilentLogin},
    {"get_display_name", GpgAuth_getDisplayName},
    {"get_id", GpgAuth_getId},
    {"is_authorized", GpgAuth_isAuthorized},
    {"set_popup_position", GpgAuth_setGravityForPopups},
    {"set_callback", Gpg_callback},
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

#define SETCONSTANT(name) \
    lua_pushnumber(L, (lua_Number) name); \
    lua_setfield(L, -2, #name); \

    SETCONSTANT(POPUP_POS_TOP_LEFT)
    SETCONSTANT(POPUP_POS_TOP_CENTER)
    SETCONSTANT(POPUP_POS_TOP_RIGHT)
    SETCONSTANT(POPUP_POS_CENTER_LEFT)
    SETCONSTANT(POPUP_POS_CENTER)
    SETCONSTANT(POPUP_POS_CENTER_RIGHT)
    SETCONSTANT(POPUP_POS_BOTTOM_LEFT)
    SETCONSTANT(POPUP_POS_BOTTOM_CENTER)
    SETCONSTANT(POPUP_POS_BOTTOM_RIGHT)

    SETCONSTANT(MSG_SIGN_IN)
    SETCONSTANT(MSG_SILENT_SIGN_IN)
    SETCONSTANT(MSG_SIGN_OUT)

    SETCONSTANT(STATUS_SUCCESS)
    SETCONSTANT(STATUS_FAILED)
    
#undef SETCONSTANT
    
    lua_pop(L,  1);
}

static void InitializeJNI()
{
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    ClassLoader class_loader = ClassLoader(env);
    jclass cls = class_loader.load("com.defold.gpgs.GpgsJNI");

    //authorization
    g_gpgs.m_silentLogin = env->GetMethodID(cls, "silentLogin", "()V");
    g_gpgs.m_login = env->GetMethodID(cls, "login", "()V");
    g_gpgs.m_logout = env->GetMethodID(cls, "logout", "()V");
    g_gpgs.m_getDisplayName = env->GetMethodID(cls, "getDisplayName", "()Ljava/lang/String;");
    g_gpgs.m_getId = env->GetMethodID(cls, "getId", "()Ljava/lang/String;");
    g_gpgs.m_setGravityForPopups = env->GetMethodID(cls, "setGravityForPopups", "(I)V");
    
    //private methods
    g_gpgs.m_activityResult = env->GetMethodID(cls, "activityResult", "(IILandroid/content/Intent;)V");
    
    jmethodID jni_constructor = env->GetMethodID(cls, "<init>", "(Landroid/app/Activity;)V");
    g_gpgs.m_GpgsJNI = env->NewGlobalRef(env->NewObject(cls, jni_constructor, dmGraphics::GetNativeAndroidActivity()));
}

static dmExtension::Result InitializeGpg(dmExtension::Params* params)
{
    LuaInit(params->m_L);
    InitializeJNI();
    dmExtension::RegisterAndroidOnActivityResultListener(OnActivityResult);
    gpgs_callback_initialize();
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeGpg(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result UpdateGpg(dmExtension::Params* params)
{
    gpgs_callback_update();
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeGpg(dmExtension::Params* params)
{
    gpgs_callback_finalize();
    dmExtension::UnregisterAndroidOnActivityResultListener(OnActivityResult);
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, AppInitializeGpg, AppFinalizeGpg, InitializeGpg, UpdateGpg, 0, FinalizeGpg)

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