#pragma once

#include <dmsdk/sdk.h>
#include "gpgs_extension.h"

struct GPGS_callback
{
    GPGS_callback() : m_L(0), m_Callback(LUA_NOREF), m_Self(LUA_NOREF) {}
    lua_State* m_L;
    int m_Callback;
    int m_Self;
};

struct CallbackData
{
    MESSAGE_ID msg;
    char* json;
};

void gpgs_set_callback(lua_State* L, int pos);
void gpgs_callback_initialize();
void gpgs_callback_finalize();
void gpgs_callback_update();
void gpgs_add_to_queue(MESSAGE_ID msg, const char*json);
