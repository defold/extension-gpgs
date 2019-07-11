#define EXTENSION_NAME GpgExt
#define LIB_NAME "GpgExt"
#define MODULE_NAME "gpg"

#define DLIB_LOG_DOMAIN LIB_NAME
#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)

#include "gpgs_jni.h"
#include "private_gpgs_callback.h"
#include "com_defold_gpgs_GpgsJNI.h"
#include "utils/LuaUtils.h"

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

enum ResolutionPolicy
{
    RESOLUTION_POLICY_MANUAL =                    -1,
    RESOLUTION_POLICY_LONGEST_PLAYTIME =           1,
    RESOLUTION_POLICY_LAST_KNOWN_GOOD =            2,
    RESOLUTION_POLICY_MOST_RECENTLY_MODIFIED =     3,
    RESOLUTION_POLICY_HIGHEST_PROGRESS =           4
};


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

struct GPGS_Disk
{
    bool                   is_using;
    
    jmethodID              m_showSavedGamesUI;
    jmethodID              m_loadSnapshot;
    jmethodID              m_getSave;
};

static GPGS         g_gpgs;
static GPGS_Disk    g_gpgs_disk;

// GPGPS autorization 

static int GpgAuth_Login(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_login);
    
    return 0;
}

static int GpgAuth_Logout(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_logout);
    
    return 0;
}

static int GpgAuth_SilentLogin(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_silentLogin);
    
    return 0;
}

static int GpgAuth_getDisplayName(lua_State* L)
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

static int GpgAuth_getId(lua_State* L)
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

static int GpgAuth_isAuthorized(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    jboolean return_value = (jboolean)env->CallBooleanMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_isAuthorized);
    
    lua_pushboolean(L, JNI_TRUE == return_value);

    return 1;
}

static int GpgAuth_setGravityForPopups(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;

    int position_lua = luaL_checknumber(L, 1);

    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_setGravityForPopups, position_lua);

    return 0;
}

static int Gpg_set_callback(lua_State* L)
{
    gpgs_set_callback(L, 1);
    return 0;
}

// GPGPS disk

static bool is_disk_avaliable()
{
    if (g_gpgs_disk.is_using)
    {
        return true;
    }
    else
    {
        dmLogWarning("GPGS Disk wasn't activated. Please check your game.project settings.");
        return false;
    }
}

static int Gpg_disk_display_saved_snapshots(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    if (not is_disk_avaliable())
    {
        return 0;
    }

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    
    int type = lua_type(L, 1);
    char* popupTitle = "Game Saves";

    if (type != LUA_TNONE && type != LUA_TNIL)
    {
        popupTitle = (char*)luaL_checkstring(L, 1);
    }

    type = lua_type(L, 2);
    bool allowAddButton = true;

    if (type != LUA_TNONE && type != LUA_TNIL)
    {
        allowAddButton = luaL_checkbool(L, 2);
    }

    type = lua_type(L, 3);
    bool allowDelete = true;

    if (type != LUA_TNONE && type != LUA_TNIL)
    {
        allowDelete = luaL_checkbool(L, 3);
    }

    type = lua_type(L, 4);
    int maxNumberOfSavedGamesToShow = 5;

    if (type != LUA_TNONE && type != LUA_TNIL)
    {
        maxNumberOfSavedGamesToShow = luaL_checknumber(L, 4);
    }
    
    jstring jpopupTitle = env->NewStringUTF(popupTitle);
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_showSavedGamesUI, jpopupTitle, allowAddButton, allowDelete, maxNumberOfSavedGamesToShow);
    env->DeleteLocalRef(jpopupTitle);

    return 0;
}

static int Gpg_disk_open_snapshot(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    if (not is_disk_avaliable())
    {
        return 0;
    }

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;

    const char* saveName = luaL_checkstring(L, 1);

    int type = lua_type(L, 2);
    bool createIfNotFound = false;

    if (type != LUA_TNONE && type != LUA_TNIL)
    {
        createIfNotFound = luaL_checkbool(L, 2);
    }

    type = lua_type(L, 3);
    int conflictPolicy = RESOLUTION_POLICY_LAST_KNOWN_GOOD;

    if (type != LUA_TNONE && type != LUA_TNIL)
    {
        conflictPolicy = luaL_checknumber(L, 3);
    }

    jstring jsaveName = env->NewStringUTF(saveName);
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_loadSnapshot, jsaveName, createIfNotFound, conflictPolicy);
    env->DeleteLocalRef(jsaveName);
    
    return 0;
}

static int Gpg_disk_save_snapshot(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }
    
    return 0;
}

static int Gpg_disk_get_snapshot(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;

    int lenght = 0;
    jbyte* cookie = NULL;

    jbyteArray cookieBArray = (jbyteArray)env->CallObjectMethod(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_getSave);

    if(cookieBArray != NULL)
    {
        DM_LUA_STACK_CHECK(L, 1);
        lenght = env->GetArrayLength(cookieBArray);
        cookie = env->GetByteArrayElements(cookieBArray, NULL);
        lua_pushlstring(L, (const char *)cookie, lenght);
        env->ReleaseByteArrayElements(cookieBArray, cookie, 0);
        return 1;
    }
    DM_LUA_STACK_CHECK(L, 2);

    lua_pushnil(L);
    lua_pushfstring(L, "Failed to load snapshot.");
    return 2;
}

static int Gpg_disk_set_snapshot(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }
    
    return 0;
}

// Extention methods

static void OnActivityResult(void *env, void* activity, int32_t request_code, int32_t result_code, void* result)
{
    ThreadAttacher attacher;
    JNIEnv *_env = attacher.env;

    _env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_activityResult, request_code, result_code, result);
}

JNIEXPORT void JNICALL Java_com_defold_gpgs_GpgsJNI_gpgsAddToQueue(JNIEnv * env, jclass cls, jint jmsg, jstring jjson)
{
    const char* json = env->GetStringUTFChars(jjson, 0);
    gpgs_add_to_queue((MESSAGE_ID)jmsg, json);
    env->ReleaseStringUTFChars(jjson, json);
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
    {"set_callback", Gpg_set_callback},
    //disk
    {"display_saved_snapshots", Gpg_disk_display_saved_snapshots},
    {"open_snapshot", Gpg_disk_open_snapshot},
    {"save_snapshot", Gpg_disk_save_snapshot},
    {"get_snapshot", Gpg_disk_get_snapshot},
    {"set_snapshot", Gpg_disk_set_snapshot},
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
    SETCONSTANT(STATUS_SNAPSHOT_COMMIT_FAILED)
    SETCONSTANT(STATUS_SNAPSHOT_CONFLICT_MISSING)
    SETCONSTANT(STATUS_SNAPSHOT_CONTENTS_UNAVAILABLE)
    SETCONSTANT(STATUS_SNAPSHOT_CREATION_FAILED)
    SETCONSTANT(STATUS_SNAPSHOT_FOLDER_UNAVAILABLE)
    SETCONSTANT(STATUS_SNAPSHOT_NOT_FOUND)

    SETCONSTANT(RESOLUTION_POLICY_MANUAL)
    SETCONSTANT(RESOLUTION_POLICY_LONGEST_PLAYTIME)
    SETCONSTANT(RESOLUTION_POLICY_LAST_KNOWN_GOOD)
    SETCONSTANT(RESOLUTION_POLICY_MOST_RECENTLY_MODIFIED)
    SETCONSTANT(RESOLUTION_POLICY_HIGHEST_PROGRESS)
    
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

    //disk
    if (g_gpgs_disk.is_using) 
    {
        g_gpgs_disk.m_showSavedGamesUI = env->GetMethodID(cls, "showSavedGamesUI", "(Ljava/lang/String;ZZI)V");
        g_gpgs_disk.m_loadSnapshot = env->GetMethodID(cls, "loadSnapshot", "(Ljava/lang/String;ZI)V");
        g_gpgs_disk.m_getSave = env->GetMethodID(cls, "getSave", "()[B");
    }
    
    //private methods
    g_gpgs.m_activityResult = env->GetMethodID(cls, "activityResult", "(IILandroid/content/Intent;)V");
    
    jmethodID jni_constructor = env->GetMethodID(cls, "<init>", "(Landroid/app/Activity;Z)V");
    g_gpgs.m_GpgsJNI = env->NewGlobalRef(env->NewObject(cls, jni_constructor, dmGraphics::GetNativeAndroidActivity(), g_gpgs_disk.is_using));
}

static dmExtension::Result InitializeGpg(dmExtension::Params* params)
{
    LuaInit(params->m_L);

    int is_using = dmConfigFile::GetInt(params->m_ConfigFile, "gpgs.use_disk", 0);
    if (is_using > 0)
    {
        g_gpgs_disk.is_using = true;
    }
    
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