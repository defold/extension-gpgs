#if defined(DM_PLATFORM_ANDROID)

#include <dmsdk/sdk.h>

//auth
void GpgAuth_Init();

int GpgAuth_Login(lua_State* L);
int GpgAuth_Logout(lua_State* L);
int GpgAuth_SilentLogin(lua_State* L);


#endif