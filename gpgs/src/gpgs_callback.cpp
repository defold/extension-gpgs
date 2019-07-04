#if defined(DM_PLATFORM_IOS) || defined(DM_PLATFORM_ANDROID)
#include "private_gpgs_callback.h"
#include "utils/LuaUtils.h"

static GPGS_callback            m_callback;
static dmArray<CallbackData>    m_callbacksQueue;
static dmMutex::HMutex          m_mutex;

static void RegisterCallback(lua_State* L, int index)
{
    GPGS_callback *cbk = &m_callback;
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

static void UnregisterCallback()
{
    GPGS_callback *cbk = &m_callback;
    if(cbk->m_Callback != LUA_NOREF)
    {
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Callback);
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Self);
        cbk->m_Callback = LUA_NOREF;
    }
}

static void gpgs_invoke_callback(int type, char*key_1, int value_1, char*key_2, char*value_2)
{
    GPGS_callback *cbk = &m_callback;
    if(cbk->m_Callback == LUA_NOREF)
    {
        dmLogInfo("GPGS callback do not exist.");
        return;
    }

    lua_State* L = cbk->m_L;
    int top = lua_gettop(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, cbk->m_Callback);
    lua_rawgeti(L, LUA_REGISTRYINDEX, cbk->m_Self);
    lua_pushvalue(L, -1);
    dmScript::SetInstance(L);

    if (!dmScript::IsInstanceValid(L))
    {
        UnregisterCallback();
        dmLogError("Could not run GPGS callback because the instance has been deleted.");
        lua_pop(L, 2);
    }
    else {
        lua_pushnumber(L, type);
        int count_table_elements = 1;
        if (key_2 != NULL & key_1 != NULL)
        {
            count_table_elements = 2;
        }
        lua_createtable(L, 0, count_table_elements);
        if (key_1 != NULL)
        {
            luaL_push_pair_str_num(L, key_1, value_1);
        }
        if (key_2 != NULL)
        {
            luaL_push_pair_str_str(L, key_2, value_2);
        }

        int number_of_arguments = 3;
        int ret = lua_pcall(L, number_of_arguments, 0, 0);
        if(ret != 0)
        {
            dmLogError("Error running callback: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }
    assert(top == lua_gettop(L));
}

void gpgs_callback_initialize()
{
    m_mutex = dmMutex::New();
}

void gpgs_callback_finalize()
{
    dmMutex::Delete(m_mutex);
    UnregisterCallback();
}

void gpgs_set_callback(lua_State* L, int pos)
{
    int type = lua_type(L, pos);
    if (type == LUA_TNONE || type == LUA_TNIL)
    {
        UnregisterCallback();
    }
    else
    {
        RegisterCallback(L, pos);
    }
}

void gpgs_add_to_queue(int msg, const char*key_1, int value_1, const char*key_2, const char*value_2)
{
    DM_MUTEX_SCOPED_LOCK(m_mutex);

    CallbackData data;
    data.msg = msg;
    data.key_1 = key_1 ? strdup(key_1) : NULL;
    data.value_1 = value_1;
    data.key_2 = key_2 ? strdup(key_2) : NULL;
    data.value_2 = value_2 ? strdup(value_2) : NULL;

    if(m_callbacksQueue.Full())
    {
        m_callbacksQueue.OffsetCapacity(1);
    }
    m_callbacksQueue.Push(data);
}

void gpgs_callback_update()
{
    if (m_callbacksQueue.Empty())
    {
        return;
    }

    DM_MUTEX_SCOPED_LOCK(m_mutex);
    
    for(uint32_t i = 0; i != m_callbacksQueue.Size(); ++i)
    {
        CallbackData* data = &m_callbacksQueue[i];
        gpgs_invoke_callback(data->msg, data->key_1, data->value_1, data->key_2, data->value_2);
        if(data->key_1)
        {
            free(data->key_1);
            data->key_1 = 0;
        }
        if(data->key_2)
        {
            free(data->key_2);
            free(data->value_2);
            data->key_2 = 0;
            data->value_1 = 0;
        }
        m_callbacksQueue.EraseSwap(i--);
    }
    
}
#endif