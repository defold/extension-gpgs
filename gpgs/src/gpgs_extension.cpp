#define EXTENSION_NAME GpgsExt
#define LIB_NAME "GpgsExt"
#define MODULE_NAME "gpgs"

#define DLIB_LOG_DOMAIN LIB_NAME
#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)

#include <string.h>

#include "gpgs_jni.h"
#include "private_gpgs_callback.h"
#include "com_defold_gpgs_GpgsJNI.h"
#include "utils/LuaUtils.h"


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
    jmethodID               m_getIdToken;
    jmethodID               m_getServerAuthCode;
    jmethodID               m_isLoggedIn;
    jmethodID               m_setGravityForPopups;
};

struct GPGS_Disk
{
    bool                   is_using;

    jmethodID              m_showSavedGamesUI;
    jmethodID              m_loadSnapshot;
    jmethodID              m_commitAndCloseSnapshot;
    jmethodID              m_getSave;
    jmethodID              m_setSave;
    jmethodID              m_isSnapshotOpened;
    jmethodID              m_getMaxCoverImageSize;
    jmethodID              m_getMaxDataSize;
    jmethodID              m_getConflictingSave;
    jmethodID              m_resolveConflict;
};

struct GPGS_Achievement
{
    jmethodID              m_RevealAchievement;
    jmethodID              m_UnlockAchievement;
    jmethodID              m_IncrementAchievement;
    jmethodID              m_SetAchievement;
    jmethodID              m_ShowAchievements;
    jmethodID              m_GetAchievements;
};

static GPGS             g_gpgs;
static GPGS_Disk        g_gpgs_disk;
static GPGS_Achievement g_gpgs_achievement;

// generic JNI calls

// void method(char*)
static void CallVoidMethodChar(jobject instance, jmethodID method, const char* cstr)
{
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    jstring jstr = env->NewStringUTF(cstr);
    env->CallVoidMethod(instance, method, jstr);
    env->DeleteLocalRef(jstr);
}

// void method(char*, int)
static void CallVoidMethodCharInt(jobject instance, jmethodID method, const char* cstr, int i)
{
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    jstring jstr = env->NewStringUTF(cstr);
    env->CallVoidMethod(instance, method, jstr, i);
    env->DeleteLocalRef(jstr);
}

// void method(int)
static void CallVoidMethodInt(jobject instance, jmethodID method, int i)
{
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    env->CallVoidMethod(instance, method, i);
}

// void method()
static int CallVoidMethod(jobject instance, jmethodID method)
{
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    env->CallVoidMethod(instance, method);
    return 0;
}

// string method()
static int CallStringMethod(lua_State* L, jobject instance, jmethodID method)
{
    DM_LUA_STACK_CHECK(L, 1);
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    jstring return_value = (jstring)env->CallObjectMethod(instance, method);
    if (return_value)
    {
        const char* cstr = env->GetStringUTFChars(return_value, 0);
        lua_pushstring(L, cstr);
        env->ReleaseStringUTFChars(return_value, cstr);
        env->DeleteLocalRef(return_value);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

// boolean method()
static int CallBooleanMethod(lua_State* L, jobject instance, jmethodID method)
{
    DM_LUA_STACK_CHECK(L, 1);
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    jboolean return_value = (jboolean)env->CallBooleanMethod(instance, method);
    lua_pushboolean(L, JNI_TRUE == return_value);
    return 1;
}

// int method()
static int CallIntMethod(lua_State* L, jobject instance, jmethodID method)
{
    DM_LUA_STACK_CHECK(L, 1);
    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    int return_value = (int)env->CallIntMethod(instance, method);
    lua_pushnumber(L, return_value);
    return 1;
}


//******************************************************************************
// GPGPS authorization
//******************************************************************************

static int GpgAuth_Login(lua_State* L)
{
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_login);
    return 0;
}

static int GpgAuth_Logout(lua_State* L)
{
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_logout);
    return 0;
}

static int GpgAuth_SilentLogin(lua_State* L)
{
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_silentLogin);
    return 0;
}

static int GpgAuth_getDisplayName(lua_State* L)
{
    return CallStringMethod(L, g_gpgs.m_GpgsJNI, g_gpgs.m_getDisplayName);
}

static int GpgAuth_getId(lua_State* L)
{
    return CallStringMethod(L, g_gpgs.m_GpgsJNI, g_gpgs.m_getId);
}

static int GpgAuth_getIdToken(lua_State* L)
{
    return CallStringMethod(L, g_gpgs.m_GpgsJNI, g_gpgs.m_getIdToken);
}

static int GpgAuth_getServerAuthCode(lua_State* L)
{
    return CallStringMethod(L, g_gpgs.m_GpgsJNI, g_gpgs.m_getServerAuthCode);
}

static int GpgAuth_isLoggedIn(lua_State* L)
{
    return CallBooleanMethod(L, g_gpgs.m_GpgsJNI, g_gpgs.m_isLoggedIn);
}

//******************************************************************************
// GPGPS misc
//******************************************************************************

static int GpgAuth_setGravityForPopups(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int position_lua = luaL_checknumber(L, 1);
    CallVoidMethodInt(g_gpgs.m_GpgsJNI, g_gpgs.m_setGravityForPopups, position_lua);
    return 0;
}

static int Gpg_set_callback(lua_State* L)
{
    gpgs_set_callback(L, 1);
    return 0;
}

//******************************************************************************
// GPGPS disk
//******************************************************************************


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

static int GpgDisk_SnapshotDisplaySaves(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    DM_LUA_STACK_CHECK(L, 0);

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;

    const char* popupTitleDefault = "Game Saves";
    char* popupTitle = luaL_checkstringd(L, 1, popupTitleDefault);
    bool allowAddButton = luaL_checkboold(L, 2, true);
    bool allowDelete = luaL_checkboold(L, 3, true);
    int maxNumberOfSavedGamesToShow = luaL_checknumberd(L, 4, 5);

    jstring jpopupTitle = env->NewStringUTF(popupTitle);
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_showSavedGamesUI, jpopupTitle, allowAddButton, allowDelete, maxNumberOfSavedGamesToShow);
    env->DeleteLocalRef(jpopupTitle);

    return 0;
}

static int GpgDisk_SnapshotOpen(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    DM_LUA_STACK_CHECK(L, 0);

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;

    const char* saveName = luaL_checkstring(L, 1);
    bool createIfNotFound = luaL_checkboold(L, 2, false);
    int conflictPolicy = luaL_checknumberd(L, 3, RESOLUTION_POLICY_LAST_KNOWN_GOOD);

    jstring jsaveName = env->NewStringUTF(saveName);
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_loadSnapshot, jsaveName, createIfNotFound, conflictPolicy);
    env->DeleteLocalRef(jsaveName);

    return 0;
}

static int GpgDisk_SnapshotCommitAndClose(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    DM_LUA_STACK_CHECK(L, 0);

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;

    long playedTime = luaL_checktable_number(L, 1, "playedTime", -1);
    long progressValue = luaL_checktable_number(L, 1, "progressValue", -1);
    char *description = luaL_checktable_string(L, 1, "description", NULL);
    char *coverImage = luaL_checktable_string(L, 1, "coverImage", NULL);
    jstring jdescription = NULL;
    if (description)
    {
        jdescription = env->NewStringUTF(description);
    }
    jbyteArray jcoverImage = NULL;
    if (coverImage)
    {
        size_t length = strlen(coverImage);
        jcoverImage = env->NewByteArray(length);
        env->SetByteArrayRegion(jcoverImage, 0, length, (jbyte*)coverImage);
    }

    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_commitAndCloseSnapshot, playedTime, progressValue, jdescription, jcoverImage);

    if (jdescription)
    {
        env->DeleteLocalRef(jdescription);
    }

    return 0;
}

static int GpgDisk_SnapshotGetData(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;

    int lenght = 0;
    jbyte* snapshot = NULL;

    jbyteArray snapshotBArray = (jbyteArray)env->CallObjectMethod(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_getSave);

    if(snapshotBArray != NULL)
    {
        DM_LUA_STACK_CHECK(L, 1);
        lenght = env->GetArrayLength(snapshotBArray);
        snapshot = env->GetByteArrayElements(snapshotBArray, NULL);
        lua_pushlstring(L, (const char*)snapshot, lenght);
        env->ReleaseByteArrayElements(snapshotBArray, snapshot, 0);
        return 1;
    }
    DM_LUA_STACK_CHECK(L, 2);

    lua_pushnil(L);
    lua_pushfstring(L, "Failed to load snapshot.");
    return 2;
}

static int GpgDisk_SnapshotSetData(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    DM_LUA_STACK_CHECK(L, 2);

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;

    size_t bytes_lenght;
    const char* bytes = luaL_checklstring(L, 1, &bytes_lenght);

    jbyteArray byteArray = env->NewByteArray(bytes_lenght);
    env->SetByteArrayRegion(byteArray, 0, bytes_lenght, (jbyte*)bytes);
    jstring return_value = (jstring)env->CallObjectMethod(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_setSave, byteArray);

    if (return_value)
    {
        lua_pushboolean(L, false);
        const char* new_char = env->GetStringUTFChars(return_value, 0);
        env->DeleteLocalRef(return_value);
        lua_pushstring(L, new_char);
    }
    else
    {
        lua_pushboolean(L, true);
        lua_pushnil(L);
    }
    return 2;
}

static int GpgDisk_SnapshotIsOpened(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    return CallBooleanMethod(L, g_gpgs.m_GpgsJNI, g_gpgs_disk.m_isSnapshotOpened);
}

static int GpgDisk_GetMaxCoverImageSize(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    return CallIntMethod(L, g_gpgs.m_GpgsJNI, g_gpgs_disk.m_getMaxCoverImageSize);
}

static int GpgDisk_GetMaxDataSize(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    return CallIntMethod(L, g_gpgs.m_GpgsJNI, g_gpgs_disk.m_getMaxDataSize);
}

static int GpgDisk_SnapshotGetConflictingData(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;

    int lenght = 0;
    jbyte* snapshot = NULL;

    jbyteArray snapshotBArray = (jbyteArray)env->CallObjectMethod(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_getConflictingSave);

    if(snapshotBArray != NULL)
    {
        DM_LUA_STACK_CHECK(L, 1);
        lenght = env->GetArrayLength(snapshotBArray);
        snapshot = env->GetByteArrayElements(snapshotBArray, NULL);
        lua_pushlstring(L, (const char*)snapshot, lenght);
        env->ReleaseByteArrayElements(snapshotBArray, snapshot, 0);
        return 1;
    }
    DM_LUA_STACK_CHECK(L, 2);

    lua_pushnil(L);
    lua_pushfstring(L, "Failed to load conflicting snapshot.");
    return 2;
}

static int GpgDisk_SnapshotResolveConflict(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    DM_LUA_STACK_CHECK(L, 0);
    const char* conflictId = luaL_checkstring(L, 1);
    int snapshotId = luaL_checknumber(L, 2);

    CallVoidMethodCharInt(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_resolveConflict, conflictId, snapshotId);
    return 0;
}

//******************************************************************************
// GPGPS achievements
//******************************************************************************

static int GpgAchievement_Reveal(lua_State* L)
{
    dmLogInfo("GpgAchievement_Reveal()");
    DM_LUA_STACK_CHECK(L, 0);

    const char* achievementId = luaL_checkstring(L, 1);
    dmLogInfo("GpgAchievement_Reveal() %s", achievementId);

    CallVoidMethodChar(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_RevealAchievement, achievementId);
    return 0;
}

static int GpgAchievement_Unlock(lua_State* L)
{
    dmLogInfo("GpgAchievement_Unlock()");
    DM_LUA_STACK_CHECK(L, 0);

    const char* achievementId = luaL_checkstring(L, 1);
    dmLogInfo("GpgAchievement_Unlock() %s", achievementId);

    CallVoidMethodChar(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_UnlockAchievement, achievementId);
    return 0;
}

static int GpgAchievement_Increment(lua_State* L)
{
    dmLogInfo("GpgAchievement_Increment()");
    DM_LUA_STACK_CHECK(L, 0);

    const char* achievementId = luaL_checkstring(L, 1);
    int steps = luaL_checknumber(L, 2);
    dmLogInfo("GpgAchievement_Increment() %s %i", achievementId, steps);

    CallVoidMethodCharInt(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_UnlockAchievement, achievementId, steps);
    return 0;
}

static int GpgAchievement_Set(lua_State* L)
{
    dmLogInfo("GpgAchievement_Set()");
    DM_LUA_STACK_CHECK(L, 0);

    const char* achievementId = luaL_checkstring(L, 1);
    int steps = luaL_checknumber(L, 2);
    dmLogInfo("GpgAchievement_Set() %s %i", achievementId, steps);

    CallVoidMethodCharInt(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_SetAchievement, achievementId, steps);
    return 0;
}

static int GpgAchievement_Show(lua_State* L)
{
    dmLogInfo("GpgAchievement_Show()");
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_ShowAchievements);
    return 0;
}

static int GpgAchievement_Get(lua_State* L)
{
    dmLogInfo("GpgAchievement_Get()");
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_GetAchievements);
    return 0;
}

// Extension methods

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
    {"get_id_token", GpgAuth_getIdToken},
    {"get_server_auth_code", GpgAuth_getServerAuthCode},
    {"is_logged_in", GpgAuth_isLoggedIn},
    {"set_popup_position", GpgAuth_setGravityForPopups},
    {"set_callback", Gpg_set_callback},
    //disk
    {"snapshot_display_saves", GpgDisk_SnapshotDisplaySaves},
    {"snapshot_open", GpgDisk_SnapshotOpen},
    {"snapshot_commit_and_close", GpgDisk_SnapshotCommitAndClose},
    {"snapshot_get_data", GpgDisk_SnapshotGetData},
    {"snapshot_set_data", GpgDisk_SnapshotSetData},
    {"snapshot_is_opened", GpgDisk_SnapshotIsOpened},
    {"snapshot_get_max_image_size", GpgDisk_GetMaxCoverImageSize},
    {"snapshot_get_max_save_size", GpgDisk_GetMaxDataSize},
    {"snapshot_get_conflicting_data", GpgDisk_SnapshotGetConflictingData},
    {"snapshot_resolve_conflict", GpgDisk_SnapshotResolveConflict},
    //achievement
    {"achievement_reveal", GpgAchievement_Reveal},
    {"achievement_unlock", GpgAchievement_Unlock},
    {"achievement_set", GpgAchievement_Set},
    {"achievement_increment", GpgAchievement_Increment},
    {"achievement_show", GpgAchievement_Show},
    {"achievement_get", GpgAchievement_Get},
    {0,0}
};

static dmExtension::Result AppInitializeGpg(dmExtension::AppParams* params)
{
    dmLogInfo("Registered extension Gpgs");
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
    SETCONSTANT(MSG_SHOW_SNAPSHOTS)
    SETCONSTANT(MSG_LOAD_SNAPSHOT)

    SETCONSTANT(STATUS_SUCCESS)
    SETCONSTANT(STATUS_FAILED)
    SETCONSTANT(STATUS_CREATE_NEW_SAVE)
    SETCONSTANT(STATUS_CONFLICT)

    SETCONSTANT(ERROR_STATUS_SNAPSHOT_COMMIT_FAILED)
    SETCONSTANT(ERROR_STATUS_SNAPSHOT_CONFLICT_MISSING)
    SETCONSTANT(ERROR_STATUS_SNAPSHOT_CONTENTS_UNAVAILABLE)
    SETCONSTANT(ERROR_STATUS_SNAPSHOT_CREATION_FAILED)
    SETCONSTANT(ERROR_STATUS_SNAPSHOT_FOLDER_UNAVAILABLE)
    SETCONSTANT(ERROR_STATUS_SNAPSHOT_NOT_FOUND)

    SETCONSTANT(RESOLUTION_POLICY_MANUAL)
    SETCONSTANT(RESOLUTION_POLICY_LONGEST_PLAYTIME)
    SETCONSTANT(RESOLUTION_POLICY_LAST_KNOWN_GOOD)
    SETCONSTANT(RESOLUTION_POLICY_MOST_RECENTLY_MODIFIED)
    SETCONSTANT(RESOLUTION_POLICY_HIGHEST_PROGRESS)

    SETCONSTANT(SNAPSHOT_CURRENT)
    SETCONSTANT(SNAPSHOT_CONFLICTING)

#undef SETCONSTANT

    lua_pop(L,  1);
}


static void InitJNIMethods(JNIEnv* env, jclass cls)
{
    //authorization
    g_gpgs.m_silentLogin = env->GetMethodID(cls, "silentLogin", "()V");
    g_gpgs.m_login = env->GetMethodID(cls, "login", "()V");
    g_gpgs.m_logout = env->GetMethodID(cls, "logout", "()V");
    g_gpgs.m_isLoggedIn = env->GetMethodID(cls, "isLoggedIn", "()Z");
    g_gpgs.m_getDisplayName = env->GetMethodID(cls, "getDisplayName", "()Ljava/lang/String;");
    g_gpgs.m_getId = env->GetMethodID(cls, "getId", "()Ljava/lang/String;");
    g_gpgs.m_getIdToken = env->GetMethodID(cls, "getIdToken", "()Ljava/lang/String;");
    g_gpgs.m_getServerAuthCode = env->GetMethodID(cls, "getServerAuthCode", "()Ljava/lang/String;");
    g_gpgs.m_setGravityForPopups = env->GetMethodID(cls, "setGravityForPopups", "(I)V");

    //disk
    if (g_gpgs_disk.is_using)
    {
        g_gpgs_disk.m_showSavedGamesUI = env->GetMethodID(cls, "showSavedGamesUI", "(Ljava/lang/String;ZZI)V");
        g_gpgs_disk.m_loadSnapshot = env->GetMethodID(cls, "loadSnapshot", "(Ljava/lang/String;ZI)V");
        g_gpgs_disk.m_getSave = env->GetMethodID(cls, "getSave", "()[B");
        g_gpgs_disk.m_setSave = env->GetMethodID(cls, "setSave", "([B)Ljava/lang/String;");
        g_gpgs_disk.m_commitAndCloseSnapshot = env->GetMethodID(cls, "commitAndCloseSnapshot", "(JJLjava/lang/String;[B)V");
        g_gpgs_disk.m_isSnapshotOpened = env->GetMethodID(cls, "isSnapshotOpened", "()Z");
        g_gpgs_disk.m_getMaxCoverImageSize = env->GetMethodID(cls, "getMaxCoverImageSize", "()I");
        g_gpgs_disk.m_getMaxDataSize = env->GetMethodID(cls, "getMaxDataSize", "()I");
        g_gpgs_disk.m_getConflictingSave = env->GetMethodID(cls, "getConflictingSave", "()[B");
        g_gpgs_disk.m_resolveConflict = env->GetMethodID(cls, "resolveConflict", "(Ljava/lang/String;I)V");
    }

    //achievement
    g_gpgs_achievement.m_RevealAchievement = env->GetMethodID(cls, "revealAchievement", "(Ljava/lang/String;)V");
    g_gpgs_achievement.m_UnlockAchievement = env->GetMethodID(cls, "unlockAchievement", "(Ljava/lang/String;)V");
    g_gpgs_achievement.m_IncrementAchievement = env->GetMethodID(cls, "incrementAchievement", "(Ljava/lang/String;I)V");
    g_gpgs_achievement.m_SetAchievement = env->GetMethodID(cls, "setAchievement", "(Ljava/lang/String;I)V");
    g_gpgs_achievement.m_ShowAchievements = env->GetMethodID(cls, "showAchievements", "()V");
    g_gpgs_achievement.m_GetAchievements = env->GetMethodID(cls, "getAchievements", "()V");

    //private methods
    g_gpgs.m_activityResult = env->GetMethodID(cls, "activityResult", "(IILandroid/content/Intent;)V");
}


static void CheckInitializationParams(const char* client_id, bool request_server_auth_code, bool request_id_token)
{
    bool is_empty_client_id = client_id == 0 || strlen(client_id) == 0;

    if (is_empty_client_id && request_server_auth_code)
    {
        dmLogError("'gpgs.client_id' must be defined to request server auth code");
    }

    if (is_empty_client_id && request_id_token)
    {
        dmLogError("'gpgs.client_id' must be defined to request id token");
    }
}


static void InitializeJNI(const char* client_id, bool request_server_auth_code, bool request_id_token)
{
    CheckInitializationParams(client_id, request_server_auth_code > 0, request_id_token > 0);

    ThreadAttacher attacher;
    JNIEnv *env = attacher.env;
    ClassLoader class_loader = ClassLoader(env);
    jclass cls = class_loader.load("com.defold.gpgs.GpgsJNI");

    InitJNIMethods(env, cls);

    jmethodID jni_constructor = env->GetMethodID(cls, "<init>", "(Landroid/app/Activity;ZZZLjava/lang/String;)V");
    jstring java_client_id = env->NewStringUTF(client_id);

    g_gpgs.m_GpgsJNI = env->NewGlobalRef(env->NewObject(cls, jni_constructor, dmGraphics::GetNativeAndroidActivity(),
                                         g_gpgs_disk.is_using, request_server_auth_code, request_id_token, java_client_id));

    env->DeleteLocalRef(java_client_id);
}

static dmExtension::Result InitializeGpg(dmExtension::Params* params)
{
    LuaInit(params->m_L);

    int is_using = dmConfigFile::GetInt(params->m_ConfigFile, "gpgs.use_saved_games", 0);
    g_gpgs_disk.is_using = is_using > 0;

    int request_server_auth_code = dmConfigFile::GetInt(params->m_ConfigFile, "gpgs.request_server_auth_code", 0);
    int request_id_token = dmConfigFile::GetInt(params->m_ConfigFile, "gpgs.request_id_token", 0);

    const char* client_id = dmConfigFile::GetString(params->m_ConfigFile, "gpgs.client_id", 0);

    InitializeJNI(client_id, request_server_auth_code > 0, request_id_token > 0);
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
    dmLogInfo("Registered extension Gpgs (null)");
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeGpg(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, 0, 0, InitializeGpg, 0, 0, FinalizeGpg)

#endif
