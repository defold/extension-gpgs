#pragma once

#include <dmsdk/sdk.h>

bool luaL_checkbool(lua_State *L, int numArg);

bool luaL_checkboold(lua_State *L, int numArg, int def);
lua_Number luaL_checknumberd(lua_State *L, int numArg, lua_Number def);
char* luaL_checkstringd(lua_State *L, int numArg, const char* def);

lua_Number luaL_checktable_number(lua_State *L, int numArg, const char* field, lua_Number def);
char* luaL_checktable_string(lua_State *L, int numArg, const char* field, char* def);

void luaL_push_pair_str_num(lua_State *L, const char *key, int value);
void luaL_push_pair_str_str(lua_State *L, const char *key, const char *value);
