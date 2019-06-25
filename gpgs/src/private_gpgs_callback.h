#pragma once

#include <dmsdk/sdk.h>

enum message_id
{
	MSG_SIGN_IN
};

struct GPGS_callback
{
	GPGS_callback() : m_L(0), m_Callback(LUA_NOREF), m_Self(LUA_NOREF) {}
	lua_State* m_L;
	int m_Callback;
	int m_Self;
};

struct CallbackData
{
	int msg_type;
	char* key_1;
	char* value_1;
	char* key_2;
	int value_2;
};

void gpgs_set_callback(lua_State* L, int pos);
void gpgs_callback_initialize();
void gpgs_callback_finalize();
void gpgs_callback_update();