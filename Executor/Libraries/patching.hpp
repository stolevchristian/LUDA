#include "../Executor.h"

namespace LUDA::Library
{
    static int c_get_bytes(lua_State* L)
    {
        if (lua_type(L, 1) == LUA_TNUMBER && lua_type(L, 2) == LUA_TNUMBER)
        {
            ea_t addr = (ea_t)lua_tointeger(L, 1);
            size_t len = (size_t)lua_tointeger(L, 2);

            // Create a new table to hold the bytes
            lua_newtable(L);

            // Read each byte and add to table
            for (size_t i = 0; i < len; i++)
            {
                uint8_t byte = get_byte(addr + i);  // IDA's get_byte function

                lua_pushinteger(L, byte);
                lua_rawseti(L, -2, i + 1);  // table[i+1] = byte (Lua is 1-indexed)
            }

            return 1;  // Return the table
        }

        lua_pushnil(L);  // Return nil on invalid args
        return 1;
    }

    static int c_patch_bytes(lua_State* L)
    {
        if (lua_type(L, 1) == LUA_TNUMBER && lua_type(L, 2) == LUA_TTABLE)
        {
            ea_t addr = (ea_t)lua_tointeger(L, 1);

            // Get the table length
            size_t len = lua_objlen(L, 2);

            // Iterate through the table and patch each byte
            for (size_t i = 1; i <= len; i++)
            {
                lua_rawgeti(L, 2, i);  // Push table[i] onto stack

                if (lua_type(L, -1) == LUA_TNUMBER)
                {
                    uint8_t byte = (uint8_t)lua_tointeger(L, -1);
                    patch_byte(addr + (i - 1), byte);  // IDA's patch function
                }

                lua_pop(L, 1);  // Pop the value from stack
            }

            lua_pushboolean(L, 1);  // Return true on success
            return 1;
        }

        lua_pushboolean(L, 0);  // Return false on invalid args
        return 1;
    }

    // Get the image base (preferred load address)
    static int c_get_imagebase(lua_State* L)
    {
        ea_t base = get_imagebase();
        lua_pushinteger(L, (lua_Number)base);
        return 1;
    }

    // Get the first address of the loaded binary (min EA)
    static int c_get_first_address(lua_State* L)
    {
        ea_t first = inf_get_min_ea();
        lua_pushinteger(L, (lua_Integer)first);
        return 1;
    }

    // Get the last address of the loaded binary (max EA)
    static int c_get_last_address(lua_State* L)
    {
        ea_t last = inf_get_max_ea();
        lua_pushinteger(L, (lua_Integer)last);
        return 1;
    }
}