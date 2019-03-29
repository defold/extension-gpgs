#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)

#include <jni.h>

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
};

static Gpg* g_Gpg = 0;

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

static JNIEnv* Attach()
{
    JNIEnv* env = 0;
    g_AndroidApp->activity->vm->AttachCurrentThread(&env, NULL);
    return env;
}

static void Detach()
{
    g_AndroidApp->activity->vm->DetachCurrentThread();
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
        lua_pushboolean(L, true);
    } else {
        dmLogWarning("Authorization already in progress")
        lua_pushboolean(L, false);
    }

    return 1;
}

static int Logout(lua_State* L)
{
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
    return 1;
}

static int ShowLeaderboard(lua_State* L)
{
    const char* leaderboard = luaL_checkstring(L, 1);
    return 0;
}

static int SubmitScore(lua_State* L)
{
    const char* leaderboard = luaL_checkstring(L, 1);
    int score = luaL_checkint(L, 2);
    return 0;
}

static int ShowAchievements(lua_State* L)
{
    return 0;
}

static int UnlockAchievement(lua_State* L)
{
    const char* achievement = luaL_checkstring(L, 1);
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

static dmExtension::Result AppInitializeGpg(dmExtension::AppParams* params)
{
    dmLogInfo("Registered extension Gpg");

    return dmExtension::RESULT_OK;
}

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);
    luaL_register(L, MODULE_NAME, Gpg_methods);
    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result InitializeGpg(dmExtension::Params* params)
{
    dmLogInfo("Initializing extension Gpg");


    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeGpg(dmExtension::AppParams* params)
{
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
