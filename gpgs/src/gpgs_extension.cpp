// C++ Documentation
// https://developers.google.com/games/services/cpp/api/class/gpg/game-services
// Getting Started
// https://developers.google.com/games/services/cpp/GettingStartedNativeClient

#include <dmsdk/sdk.h>
#include <assert.h>

#if defined(DM_PLATFORM_ANDROID)

#include <gpg/android_initialization.h>
#include <gpg/android_support.h>
#include <gpg/achievement.h>
#include <gpg/achievement_manager.h>
#include <gpg/types.h>
#include <gpg/builder.h>
#include <gpg/debug.h>
#include <gpg/default_callbacks.h>
#include <gpg/game_services.h>
#include <gpg/leaderboard_manager.h>
#include <gpg/achievement_manager.h>

#if defined(DM_PLATFORM_ANDROID)
    #include <android_native_app_glue.h>
#endif


#ifdef DLIB_LOG_DOMAIN
#undef DLIB_LOG_DOMAIN
#define DLIB_LOG_DOMAIN "GPG"
#endif
#define LIB_NAME GpgExt
#define MODULE_NAME "gpg"

struct LuaCallbackInfo
{
    LuaCallbackInfo() : m_L(0), m_Callback(LUA_NOREF), m_Self(LUA_NOREF) {
        dmLogWarning("LuaCallbackInfo constructor");
    }
    lua_State* m_L;
    int        m_Callback;
    int        m_Self;
};

struct Gpg
{
    Gpg() : m_AuthInProgress(false) {}
    bool m_AuthInProgress;
    LuaCallbackInfo m_Callback;
    std::unique_ptr<gpg::GameServices> m_GameServices;
};

static Gpg* g_Gpg = 0;

#define CHECK_GPG() {if(!g_Gpg || !g_Gpg->m_GameServices) return luaL_error(L, "GPG not initialized!");}

static void RegisterCallback(lua_State* L, int index, LuaCallbackInfo* cbk)
{
    if(cbk->m_Callback != LUA_NOREF)
    {
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Callback);
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Self);
    }

    cbk->m_L = dmScript::GetMainThread(L);

    luaL_checktype(L, index, LUA_TFUNCTION);
    lua_pushvalue(L, index);
    cbk->m_Callback = dmScript::Ref(L, LUA_REGISTRYINDEX);

    dmScript::GetInstance(L);
    cbk->m_Self = dmScript::Ref(L, LUA_REGISTRYINDEX);
}

static void UnregisterCallback(LuaCallbackInfo* cbk)
{
    if(cbk->m_Callback != LUA_NOREF)
    {
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Callback);
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Self);
        cbk->m_Callback = LUA_NOREF;
    }
}

static void InvokeCallbackAuthOperation(LuaCallbackInfo* cbk, gpg::AuthOperation auth_operation, gpg::AuthStatus status)
{
    if(cbk->m_Callback == LUA_NOREF)
        return;

    lua_State* L = cbk->m_L;
    int top = lua_gettop(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, cbk->m_Callback);

    // Setup self (the script instance)
    lua_rawgeti(L, LUA_REGISTRYINDEX, cbk->m_Self);
    lua_pushvalue(L, -1);

    dmScript::SetInstance(L);

    dmLogWarning("GPG InvokeCallback: %d %d", auth_operation, status);
    lua_pushinteger(L, (int)auth_operation);
    lua_pushinteger(L, (int)status);

    int number_of_arguments = 3; // instance + 2
    int ret = lua_pcall(L, number_of_arguments, 0, 0);
    if(ret != 0) {
        dmLogError("Error running callback: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    assert(top == lua_gettop(L));
}


static void OnAuthActionStarted(gpg::AuthOperation auth_operation)
{
    dmLogWarning("GPG OnAuthActionStarted: %d", auth_operation);
    g_Gpg->m_AuthInProgress = true;

    switch ( auth_operation ) {
      case gpg::AuthOperation::SIGN_IN:
        dmLogWarning("Signing In");
        break;
      case gpg::AuthOperation::SIGN_OUT:
        dmLogWarning("Signing Out");
        break;
    }
}

static void OnAuthActionFinished(gpg::AuthOperation auth_operation, gpg::AuthStatus status)
{
    g_Gpg->m_AuthInProgress = false;
    // TODO: Log warning if status == -3 (NOT_AUTH). Probably wrong certificate
    // OR maybe NOT! We can this on startup as well the first time (autologin)
    dmLogWarning("OnAuthActionFinished: %s (%d) %s (%d)", gpg::DebugString(auth_operation).c_str(), auth_operation, gpg::DebugString(status).c_str(), status);
    dmLogWarning("IsAuthorized: %d", g_Gpg->m_GameServices->IsAuthorized());

    InvokeCallbackAuthOperation(&g_Gpg->m_Callback, auth_operation, status);
}

/** Login
 * @param callback  The callback of the form "auth_status_callback(self, auth_operation, auth_status)"
 */
static int Login(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    dmLogWarning("LOGIN %p %d %d", g_Gpg->m_Callback.m_L, g_Gpg->m_Callback.m_Self, g_Gpg->m_Callback.m_Callback);

    RegisterCallback(L, 1, &g_Gpg->m_Callback);

    if (!g_Gpg->m_AuthInProgress) {
        g_Gpg->m_GameServices->StartAuthorizationUI();
        lua_pushboolean(L, true);
    } else {
        dmLogWarning("Authorization already in progress")
        lua_pushboolean(L, false);
    }

    return 1;
}

static int Logout(lua_State* L)
{
    CHECK_GPG();
    g_Gpg->m_GameServices->SignOut();
    return 0;
}

static int IsAuthInProgress(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, g_Gpg ? g_Gpg->m_AuthInProgress : false);
    return 1;
}

static int IsAuthorized(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, g_Gpg && g_Gpg->m_GameServices ? g_Gpg->m_GameServices->IsAuthorized() : false);
    return 1;
}

static int ShowLeaderboard(lua_State* L)
{
    CHECK_GPG();
    const char* leaderboard = luaL_checkstring(L, 1);
    g_Gpg->m_GameServices->Leaderboards().ShowUI(leaderboard);
    return 0;
}

static int SubmitScore(lua_State* L)
{
    CHECK_GPG();
    const char* leaderboard = luaL_checkstring(L, 1);
    int score = luaL_checkint(L, 2);
    g_Gpg->m_GameServices->Leaderboards().SubmitScore(leaderboard, (uint64_t) score);
    return 0;
}

static int ShowAchievements(lua_State* L)
{
    CHECK_GPG();
    g_Gpg->m_GameServices->Achievements().ShowAllUI();
    return 0;
}

static int UnlockAchievement(lua_State* L)
{
    CHECK_GPG();
    const char* achievement = luaL_checkstring(L, 1);
    g_Gpg->m_GameServices->Achievements().Unlock(achievement);
    return 0;
}

static const luaL_reg Gpg_methods[] =
{
    {"login", Login},
    {"logout", Logout},
    {"is_auth_in_progress", IsAuthInProgress},
    {"is_authorized", IsAuthorized},
    {"show_leaderboard", ShowLeaderboard},
    {"submit_score", SubmitScore},
    {"show_achievements", ShowAchievements},
    {"unlock_achievement", UnlockAchievement},
    {0,0}
};

static const luaL_reg Gpg_meta[] =
{
    {0, 0}
};

static void OnActivityResult(void *env, void* activity, int32_t request_code, int32_t result_code, void* result)
{
    dmLogInfo("OnActivityResult: env: %p  activity: %p  request_code: %d  result_code: %d  result: %p", env, activity, request_code, result_code, result);

    gpg::AndroidSupport::OnActivityResult((JNIEnv*) env, (jobject) activity, request_code, result_code, (jobject) result);
}

static dmExtension::Result AppInitializeGpg(dmExtension::AppParams* params)
{
    dmLogInfo("Registered extension Gpg");

    return dmExtension::RESULT_OK;
}

static void LuaInit(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    luaL_register(L, MODULE_NAME, Gpg_methods);

#define SETAUTHOPERATIONCONSTANT(name) \
    lua_pushinteger(L, (int) gpg::AuthOperation:: name); \
    lua_setfield(L, -2, "AUTH_OPERATION_" #name); \

    SETAUTHOPERATIONCONSTANT(SIGN_IN);
    SETAUTHOPERATIONCONSTANT(SIGN_OUT);

#define SETAUTHSTATUSCONSTANT(name) \
    lua_pushinteger(L, (int) gpg::AuthStatus:: name); \
    lua_setfield(L, -2, "AUTH_STATUS_" #name); \

    SETAUTHSTATUSCONSTANT(VALID);
    SETAUTHSTATUSCONSTANT(ERROR_INTERNAL);
    SETAUTHSTATUSCONSTANT(ERROR_NOT_AUTHORIZED);
    SETAUTHSTATUSCONSTANT(ERROR_VERSION_UPDATE_REQUIRED);
    SETAUTHSTATUSCONSTANT(ERROR_TIMEOUT);
    SETAUTHSTATUSCONSTANT(ERROR_NO_DATA);
    SETAUTHSTATUSCONSTANT(ERROR_NETWORK_OPERATION_FAILED);
    SETAUTHSTATUSCONSTANT(ERROR_APP_MISCONFIGURED);
    SETAUTHSTATUSCONSTANT(ERROR_GAME_NOT_FOUND);
    SETAUTHSTATUSCONSTANT(ERROR_INTERRUPTED);

    lua_pop(L, 1);

    #undef SETAUTOOPERATIONCONSTANT
    #undef SETAUTHSTATUSCONSTANT
}

static dmExtension::Result InitializeGpg(dmExtension::Params* params)
{
    dmLogInfo("Initializing extension Gpg");

    gpg::AndroidInitialization::android_main(dmGraphics::GetNativeAndroidApp());
    dmExtension::RegisterAndroidOnActivityResultListener(OnActivityResult);

    LuaInit(params->m_L);

    g_Gpg = new Gpg;

    if (!g_Gpg->m_GameServices)
    {
        gpg::AndroidPlatformConfiguration platform_configuration;
        platform_configuration.SetActivity(dmGraphics::GetNativeAndroidActivity());

        g_Gpg->m_GameServices = gpg::GameServices::Builder()
            .SetDefaultOnLog(gpg::LogLevel::VERBOSE)
            .SetOnAuthActionStarted(OnAuthActionStarted)
            .SetOnAuthActionFinished(OnAuthActionFinished)
            .Create(platform_configuration);
    }

    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeGpg(dmExtension::AppParams* params)
{
    dmExtension::UnregisterAndroidOnActivityResultListener(OnActivityResult);

    delete g_Gpg;
    g_Gpg = 0;
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeGpg(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(LIB_NAME, "Gpg", AppInitializeGpg, AppFinalizeGpg, InitializeGpg, 0, 0, FinalizeGpg)

#else

dmExtension::Result AppInitializeGpg(dmExtension::AppParams* params)
{
    dmLogInfo("Registered extension Gpg (null)");
    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeGpg(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(LIB_NAME, "Gpg", AppInitializeGpg, AppFinalizeGpg, 0, 0, 0, 0)

#endif
