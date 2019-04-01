#define EXTENSION_NAME GpgExt
#define LIB_NAME "GpgExt"
#define MODULE_NAME "gpg"

#define DLIB_LOG_DOMAIN LIB_NAME

#if defined(DM_PLATFORM_ANDROID)

#include "gpgs_extension.h"
#include <jni.h>

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
    return 0;
}

#endif