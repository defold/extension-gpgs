#define EXTENSION_NAME GpgExt
#define LIB_NAME "GpgExt"
#define MODULE_NAME "gpg"

#define DLIB_LOG_DOMAIN LIB_NAME
#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)

#include "gpgs_extension.h"

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
    GpgAuth_Init();
}

static dmExtension::Result InitializeGpg(dmExtension::Params* params)
{
    LuaInit(params->m_L);
    InitializeJNI();
    dmLogInfo("Initializing extension Gpg");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeGpg(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeGpg(dmExtension::Params* params)
{
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