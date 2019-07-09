#pragma once

#include <dmsdk/sdk.h>

enum MESSAGE_ID
{
    MSG_SIGN_IN =            1,
    MSG_SILENT_SIGN_IN =     2,
    MSG_SIGN_OUT =           3,
    MSG_SHOW_SNAPSHOTS =     4,
    MSG_LOAD_SNAPSHOT =      5
};

enum STATUS
{
    STATUS_SUCCESS =                        1,
    STATUS_FAILED =                         2,
    STATUS_SNAPSHOT_NOT_FOUND =             26570,
    STATUS_SNAPSHOT_CREATION_FAILED =       26571,
    STATUS_SNAPSHOT_CONTENTS_UNAVAILABLE =  26572,
    STATUS_SNAPSHOT_COMMIT_FAILED =         26573,
    STATUS_SNAPSHOT_FOLDER_UNAVAILABLE =    26575,
    STATUS_SNAPSHOT_CONFLICT_MISSING =      26576,
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
    MESSAGE_ID msg;
    char* json;
};

void gpgs_set_callback(lua_State* L, int pos);
void gpgs_callback_initialize();
void gpgs_callback_finalize();
void gpgs_callback_update();
void gpgs_add_to_queue(MESSAGE_ID msg, const char*json);