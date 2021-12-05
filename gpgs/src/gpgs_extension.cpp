#define EXTENSION_NAME GpgsExt
#define LIB_NAME "GpgsExt"
#define MODULE_NAME "gpgs"

#define DLIB_LOG_DOMAIN LIB_NAME
#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)

#include <string.h>
#include <dmsdk/dlib/android.h>
#include "gpgs_extension.h"
#include "private_gpgs_callback.h"
#include "com_defold_gpgs_GpgsJNI.h"


struct GPGS
{
    jobject                 m_GpgsJNI;
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
    jmethodID               m_isSupported;
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

struct GPGS_Leaderboard
{
    jmethodID              m_SubmitScore;
    jmethodID              m_LoadTopScores;
    jmethodID              m_LoadPlayerCenteredScores;
    jmethodID              m_ShowLeaderboard;
    jmethodID              m_ShowAllLeaderboards;
    jmethodID              m_LoadCurrentPlayerScore;
};

struct GPGS_Events
{
    jmethodID              m_IncrementEvent;
    jmethodID              m_LoadEvents;
};

static GPGS             g_gpgs;
static GPGS_Disk        g_gpgs_disk;
static GPGS_Achievement g_gpgs_achievement;
static GPGS_Leaderboard g_gpgs_leaderboard;
static GPGS_Events      g_gpgs_events;


// generic utility functions


static bool luaL_checkbool(lua_State *L, int numArg)
{
    bool b = false;
    if (lua_isboolean(L, numArg))
    {
        b = lua_toboolean(L, numArg);
    }
    else
    {
        luaL_typerror(L, numArg, lua_typename(L, LUA_TBOOLEAN));
    }
    return b;
}

static bool luaL_checkboold(lua_State *L, int numArg, int def)
{
    int type = lua_type(L, numArg);
    if (type != LUA_TNONE && type != LUA_TNIL)
    {
        return luaL_checkbool(L, numArg);
    }
    return def;
}

static lua_Number luaL_checknumberd(lua_State *L, int numArg, lua_Number def)
{
    int type = lua_type(L, numArg);
    if (type != LUA_TNONE && type != LUA_TNIL)
    {
        return luaL_checknumber(L, numArg);
    }
    return def;
}

static char* luaL_checkstringd(lua_State *L, int numArg, const char* def)
{
    int type = lua_type(L, numArg);
    if (type != LUA_TNONE && type != LUA_TNIL)
    {
        return (char*)luaL_checkstring(L, numArg);
    }
    return (char*)def;
}

static lua_Number luaL_checktable_number(lua_State *L, int numArg, const char* field, lua_Number def)
{
    lua_Number result = def;
    if(lua_istable(L, numArg))
    {
        lua_getfield(L, numArg, field);
        if(!lua_isnil(L, -1))
        {
            result = luaL_checknumber(L, -1);
        }
        lua_pop(L, 1);
    }
    return result;
}

static char* luaL_checktable_string(lua_State *L, int numArg, const char* field, char* def)
{
    char* result = def;
    if(lua_istable(L, numArg))
    {
        lua_getfield(L, numArg, field);
        if(!lua_isnil(L, -1))
        {
            result = (char*)luaL_checkstring(L, -1);
        }
        lua_pop(L, 1);
    }
    return result;
}


// generic JNI calls

// void method(char*)
static void CallVoidMethodChar(jobject instance, jmethodID method, const char* cstr)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    jstring jstr = env->NewStringUTF(cstr);
    env->CallVoidMethod(instance, method, jstr);
    env->DeleteLocalRef(jstr);
}

// void method(char*, int)
static void CallVoidMethodCharInt(jobject instance, jmethodID method, const char* cstr, int i)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    jstring jstr = env->NewStringUTF(cstr);
    env->CallVoidMethod(instance, method, jstr, i);
    env->DeleteLocalRef(jstr);
}

// void method(char*, int, int)
static void CallVoidMethodCharIntInt(jobject instance, jmethodID method, const char* cstr, int i1, int i2)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    jstring jstr = env->NewStringUTF(cstr);
    env->CallVoidMethod(instance, method, jstr, i1, i2);
    env->DeleteLocalRef(jstr);
}

// void method(char*, int, int, int)
static void CallVoidMethodCharIntIntInt(jobject instance, jmethodID method, const char* cstr, int i1, int i2, int i3)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    jstring jstr = env->NewStringUTF(cstr);
    env->CallVoidMethod(instance, method, jstr, i1, i2, i3);
    env->DeleteLocalRef(jstr);
}

// void method(char*, double)
static void CallVoidMethodCharDouble(jobject instance, jmethodID method, const char* cstr, double d)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    jstring jstr = env->NewStringUTF(cstr);
    env->CallVoidMethod(instance, method, jstr, d);
    env->DeleteLocalRef(jstr);
}

// void method(int)
static void CallVoidMethodInt(jobject instance, jmethodID method, int i)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    env->CallVoidMethod(instance, method, i);
}

// void method()
static int CallVoidMethod(jobject instance, jmethodID method)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    env->CallVoidMethod(instance, method);
    return 0;
}

// string method()
static int CallStringMethod(lua_State* L, jobject instance, jmethodID method)
{
    DM_LUA_STACK_CHECK(L, 1);
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
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
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    jboolean return_value = (jboolean)env->CallBooleanMethod(instance, method);
    lua_pushboolean(L, JNI_TRUE == return_value);
    return 1;
}

// int method()
static int CallIntMethod(lua_State* L, jobject instance, jmethodID method)
{
    DM_LUA_STACK_CHECK(L, 1);
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    int return_value = (int)env->CallIntMethod(instance, method);
    lua_pushnumber(L, return_value);
    return 1;
}


//******************************************************************************
// GPGPS authorization
//******************************************************************************

static int GpgsAuth_Login(lua_State* L)
{
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_login);
    return 0;
}

static int GpgsAuth_Logout(lua_State* L)
{
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_logout);
    return 0;
}

static int GpgsAuth_SilentLogin(lua_State* L)
{
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_silentLogin);
    return 0;
}

static int GpgsAuth_getDisplayName(lua_State* L)
{
    return CallStringMethod(L, g_gpgs.m_GpgsJNI, g_gpgs.m_getDisplayName);
}

static int GpgsAuth_getId(lua_State* L)
{
    return CallStringMethod(L, g_gpgs.m_GpgsJNI, g_gpgs.m_getId);
}

static int GpgsAuth_getIdToken(lua_State* L)
{
    return CallStringMethod(L, g_gpgs.m_GpgsJNI, g_gpgs.m_getIdToken);
}

static int GpgsAuth_getServerAuthCode(lua_State* L)
{
    return CallStringMethod(L, g_gpgs.m_GpgsJNI, g_gpgs.m_getServerAuthCode);
}

static int GpgsAuth_isLoggedIn(lua_State* L)
{
    return CallBooleanMethod(L, g_gpgs.m_GpgsJNI, g_gpgs.m_isLoggedIn);
}

static int GpgsAuth_isSupported(lua_State* L)
{
    return CallBooleanMethod(L, g_gpgs.m_GpgsJNI, g_gpgs.m_isSupported);
}

//******************************************************************************
// GPGPS misc
//******************************************************************************

static int GpgsAuth_setGravityForPopups(lua_State* L)
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

static int GpgsDisk_SnapshotDisplaySaves(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    DM_LUA_STACK_CHECK(L, 0);

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

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

static int GpgsDisk_SnapshotOpen(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    DM_LUA_STACK_CHECK(L, 0);

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    const char* saveName = luaL_checkstring(L, 1);
    bool createIfNotFound = luaL_checkboold(L, 2, false);
    int conflictPolicy = luaL_checknumberd(L, 3, RESOLUTION_POLICY_LAST_KNOWN_GOOD);

    jstring jsaveName = env->NewStringUTF(saveName);
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_loadSnapshot, jsaveName, createIfNotFound, conflictPolicy);
    env->DeleteLocalRef(jsaveName);

    return 0;
}

static int GpgsDisk_SnapshotCommitAndClose(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    DM_LUA_STACK_CHECK(L, 0);

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

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

    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_disk.m_commitAndCloseSnapshot, (jlong)playedTime, (jlong)progressValue, jdescription, jcoverImage);

    if (jdescription)
    {
        env->DeleteLocalRef(jdescription);
    }

    return 0;
}

static int GpgsDisk_SnapshotGetData(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

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

static int GpgsDisk_SnapshotSetData(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    DM_LUA_STACK_CHECK(L, 2);

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

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

static int GpgsDisk_SnapshotIsOpened(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    return CallBooleanMethod(L, g_gpgs.m_GpgsJNI, g_gpgs_disk.m_isSnapshotOpened);
}

static int GpgsDisk_GetMaxCoverImageSize(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    return CallIntMethod(L, g_gpgs.m_GpgsJNI, g_gpgs_disk.m_getMaxCoverImageSize);
}

static int GpgsDisk_GetMaxDataSize(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    return CallIntMethod(L, g_gpgs.m_GpgsJNI, g_gpgs_disk.m_getMaxDataSize);
}

static int GpgsDisk_SnapshotGetConflictingData(lua_State* L)
{
    if (not is_disk_avaliable())
    {
        return 0;
    }

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

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

static int GpgsDisk_SnapshotResolveConflict(lua_State* L)
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

static int GpgsAchievement_Reveal(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    const char* achievementId = luaL_checkstring(L, 1);
    CallVoidMethodChar(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_RevealAchievement, achievementId);
    return 0;
}

static int GpgsAchievement_Unlock(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    const char* achievementId = luaL_checkstring(L, 1);
    CallVoidMethodChar(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_UnlockAchievement, achievementId);
    return 0;
}

static int GpgsAchievement_Increment(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    const char* achievementId = luaL_checkstring(L, 1);
    int steps = luaL_checknumber(L, 2);
    CallVoidMethodCharInt(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_IncrementAchievement, achievementId, steps);
    return 0;
}

static int GpgsAchievement_Set(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    const char* achievementId = luaL_checkstring(L, 1);
    int steps = luaL_checknumber(L, 2);
    CallVoidMethodCharInt(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_SetAchievement, achievementId, steps);
    return 0;
}

static int GpgsAchievement_Show(lua_State* L)
{
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_ShowAchievements);
    return 0;
}

static int GpgsAchievement_Get(lua_State* L)
{
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_achievement.m_GetAchievements);
    return 0;
}

//******************************************************************************
// GPGPS leaderboard
//******************************************************************************

static int GpgsLeaderboard_SubmitScore(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    const char* leaderboardId = luaL_checkstring(L, 1);
    lua_Number score = luaL_checknumber(L, 2);
    CallVoidMethodCharDouble(g_gpgs.m_GpgsJNI, g_gpgs_leaderboard.m_SubmitScore, leaderboardId, score);
    return 0;
}

static int GpgsLeaderboard_GetTopScores(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    const char* leaderboardId = luaL_checkstring(L, 1);
    lua_Number span = luaL_checknumber(L, 2);
    lua_Number collection = luaL_checknumber(L, 3);
    lua_Number maxResults = luaL_checknumber(L, 4);
    CallVoidMethodCharIntIntInt(g_gpgs.m_GpgsJNI, g_gpgs_leaderboard.m_LoadTopScores, leaderboardId, span, collection, maxResults);
    return 0;
}

static int GpgsLeaderboard_GetPlayerCenteredScores(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    const char* leaderboardId = luaL_checkstring(L, 1);
    lua_Number span = luaL_checknumber(L, 2);
    lua_Number collection = luaL_checknumber(L, 3);
    lua_Number maxResults = luaL_checknumber(L, 4);
    CallVoidMethodCharIntIntInt(g_gpgs.m_GpgsJNI, g_gpgs_leaderboard.m_LoadPlayerCenteredScores, leaderboardId, span, collection, maxResults);
    return 0;
}

static int GpgsLeaderboard_GetPlayerScore(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    const char* leaderboardId = luaL_checkstring(L, 1);
    lua_Number span = luaL_checknumber(L, 2);
    lua_Number collection = luaL_checknumber(L, 3);
    CallVoidMethodCharIntInt(g_gpgs.m_GpgsJNI, g_gpgs_leaderboard.m_LoadCurrentPlayerScore, leaderboardId, span, collection);
    return 0;
}

static int GpgsLeaderboard_ShowLeaderboard(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    const char* leaderboardId = luaL_checkstring(L, 1);
    lua_Number span = luaL_checknumber(L, 2);
    lua_Number collection = luaL_checknumber(L, 3);
    CallVoidMethodCharIntInt(g_gpgs.m_GpgsJNI, g_gpgs_leaderboard.m_ShowLeaderboard, leaderboardId, span, collection);
    return 0;
}

static int GpgsLeaderboard_ShowAllLeaderboards(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_leaderboard.m_ShowAllLeaderboards);
    return 0;
}

//******************************************************************************
// GPGPS Events
//******************************************************************************


static int GpgsEvent_Increment(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    const char* eventId = luaL_checkstring(L, 1);
    lua_Number amount = luaL_checknumber(L, 2);
    CallVoidMethodCharInt(g_gpgs.m_GpgsJNI, g_gpgs_events.m_IncrementEvent, eventId, amount);
    return 0;
}


static int GpgsEvent_Get(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs_events.m_LoadEvents);
    return 0;
}

// Extension methods

static void OnActivityResult(JNIEnv* env, jobject activity, int32_t request_code, int32_t result_code, void* result)
{
    env->CallVoidMethod(g_gpgs.m_GpgsJNI, g_gpgs.m_activityResult, request_code, result_code, result);
}

JNIEXPORT void JNICALL Java_com_defold_gpgs_GpgsJNI_gpgsAddToQueue(JNIEnv * env, jclass cls, jint jmsg, jstring jjson)
{
    const char* json = env->GetStringUTFChars(jjson, 0);
    gpgs_add_to_queue((MESSAGE_ID)jmsg, json);
    env->ReleaseStringUTFChars(jjson, json);
}
//-----

static const luaL_reg Gpgs_methods[] =
{
    //general
    {"is_supported", GpgsAuth_isSupported},
    //authorization
    {"login", GpgsAuth_Login},
    {"logout", GpgsAuth_Logout},
    {"silent_login", GpgsAuth_SilentLogin},
    {"get_display_name", GpgsAuth_getDisplayName},
    {"get_id", GpgsAuth_getId},
    {"get_id_token", GpgsAuth_getIdToken},
    {"get_server_auth_code", GpgsAuth_getServerAuthCode},
    {"is_logged_in", GpgsAuth_isLoggedIn},
    {"set_popup_position", GpgsAuth_setGravityForPopups},
    {"set_callback", Gpg_set_callback},
    //disk
    {"snapshot_display_saves", GpgsDisk_SnapshotDisplaySaves},
    {"snapshot_open", GpgsDisk_SnapshotOpen},
    {"snapshot_commit_and_close", GpgsDisk_SnapshotCommitAndClose},
    {"snapshot_get_data", GpgsDisk_SnapshotGetData},
    {"snapshot_set_data", GpgsDisk_SnapshotSetData},
    {"snapshot_is_opened", GpgsDisk_SnapshotIsOpened},
    {"snapshot_get_max_image_size", GpgsDisk_GetMaxCoverImageSize},
    {"snapshot_get_max_save_size", GpgsDisk_GetMaxDataSize},
    {"snapshot_get_conflicting_data", GpgsDisk_SnapshotGetConflictingData},
    {"snapshot_resolve_conflict", GpgsDisk_SnapshotResolveConflict},
    //achievement
    {"achievement_reveal", GpgsAchievement_Reveal},
    {"achievement_unlock", GpgsAchievement_Unlock},
    {"achievement_set", GpgsAchievement_Set},
    {"achievement_increment", GpgsAchievement_Increment},
    {"achievement_show", GpgsAchievement_Show},
    {"achievement_get", GpgsAchievement_Get},
    //leaderboard
    {"leaderboard_submit_score", GpgsLeaderboard_SubmitScore},
    {"leaderboard_get_top_scores", GpgsLeaderboard_GetTopScores},
    {"leaderboard_get_player_centered_scores", GpgsLeaderboard_GetPlayerCenteredScores},
    {"leaderboard_show", GpgsLeaderboard_ShowLeaderboard},
    {"leaderboard_list", GpgsLeaderboard_ShowAllLeaderboards},
    {"leaderboard_get_player_score", GpgsLeaderboard_GetPlayerScore},
    //events
    {"event_increment", GpgsEvent_Increment},
    {"event_get", GpgsEvent_Get},
    {0,0}
};

static dmExtension::Result AppInitializeGpgs(dmExtension::AppParams* params)
{
    dmLogInfo("Registered extension Gpgs");
    return dmExtension::RESULT_OK;
}

static void LuaInit(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    luaL_register(L, MODULE_NAME, Gpgs_methods);

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
    SETCONSTANT(MSG_GET_ACHIEVEMENTS)
    SETCONSTANT(MSG_GET_TOP_SCORES)
    SETCONSTANT(MSG_GET_PLAYER_CENTERED_SCORES)
    SETCONSTANT(MSG_GET_PLAYER_SCORE)
    SETCONSTANT(MSG_GET_EVENTS)

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

    SETCONSTANT(TIME_SPAN_DAILY)
    SETCONSTANT(TIME_SPAN_WEEKLY)
    SETCONSTANT(TIME_SPAN_ALL_TIME)

    SETCONSTANT(COLLECTION_PUBLIC)
    SETCONSTANT(COLLECTION_SOCIAL)


#undef SETCONSTANT

    lua_pop(L,  1);
}


static void InitJNIMethods(JNIEnv* env, jclass cls)
{
    //general
    g_gpgs.m_isSupported = env->GetMethodID(cls, "isSupported", "()Z");

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

    //leaderboard
    g_gpgs_leaderboard.m_SubmitScore = env->GetMethodID(cls, "submitScore", "(Ljava/lang/String;D)V");
    g_gpgs_leaderboard.m_LoadTopScores = env->GetMethodID(cls, "loadTopScores", "(Ljava/lang/String;III)V");
    g_gpgs_leaderboard.m_LoadPlayerCenteredScores = env->GetMethodID(cls, "loadPlayerCenteredScores", "(Ljava/lang/String;III)V");
    g_gpgs_leaderboard.m_ShowLeaderboard = env->GetMethodID(cls, "showLeaderboard", "(Ljava/lang/String;II)V");
    g_gpgs_leaderboard.m_ShowAllLeaderboards = env->GetMethodID(cls, "showAllLeaderboards", "()V");
    g_gpgs_leaderboard.m_LoadCurrentPlayerScore = env->GetMethodID(cls, "loadCurrentPlayerLeaderboardScore", "(Ljava/lang/String;II)V");

    g_gpgs_events.m_IncrementEvent = env->GetMethodID(cls, "incrementEvent", "(Ljava/lang/String;I)V");
    g_gpgs_events.m_LoadEvents = env->GetMethodID(cls, "loadEvents", "()V");

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

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    jclass cls = dmAndroid::LoadClass(env, "com.defold.gpgs.GpgsJNI");

    InitJNIMethods(env, cls);

    jmethodID jni_constructor = env->GetMethodID(cls, "<init>", "(Landroid/app/Activity;ZZZLjava/lang/String;)V");
    jstring java_client_id = env->NewStringUTF(client_id);

    g_gpgs.m_GpgsJNI = env->NewGlobalRef(env->NewObject(cls, jni_constructor, threadAttacher.GetActivity()->clazz,
                                g_gpgs_disk.is_using, request_server_auth_code, request_id_token, java_client_id));

    env->DeleteLocalRef(java_client_id);
}

static dmExtension::Result InitializeGpgs(dmExtension::Params* params)
{
    LuaInit(params->m_L);

    int is_using = dmConfigFile::GetInt(params->m_ConfigFile, "gpgs.use_saved_games", 0);
    g_gpgs_disk.is_using = is_using > 0;

    int request_server_auth_code = dmConfigFile::GetInt(params->m_ConfigFile, "gpgs.request_server_auth_code", 0);
    int request_id_token = dmConfigFile::GetInt(params->m_ConfigFile, "gpgs.request_id_token", 0);

    const char* client_id = dmConfigFile::GetString(params->m_ConfigFile, "gpgs.client_id", 0);

    InitializeJNI(client_id, request_server_auth_code > 0, request_id_token > 0);
    dmAndroid::RegisterOnActivityResultListener(OnActivityResult);
    gpgs_callback_initialize();
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeGpgs(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result UpdateGpgs(dmExtension::Params* params)
{
    gpgs_callback_update();
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeGpgs(dmExtension::Params* params)
{
    gpgs_callback_finalize();
    dmAndroid::UnregisterOnActivityResultListener(OnActivityResult);
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, AppInitializeGpgs, AppFinalizeGpgs, InitializeGpgs, UpdateGpgs, 0, FinalizeGpgs)

#else

dmExtension::Result InitializeGpgs(dmExtension::Params* params)
{
    dmLogInfo("Registered extension Gpgs (null)");
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeGpgs(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, 0, 0, InitializeGpgs, 0, 0, FinalizeGpgs)

#endif
