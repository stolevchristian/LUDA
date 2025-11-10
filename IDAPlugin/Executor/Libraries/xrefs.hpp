#include "../Executor.h"

namespace LUDA::Library
{
    static int c_get_xrefs(lua_State* L)
    {
        ea_t target_addr = lua_tonumber(L, 1);

        lua_newtable(L);
        int i = 0;

        // Get code xrefs
        ea_t xref_addr = get_first_cref_to(target_addr);
        while (xref_addr != BADADDR) {
            i += 1;
            lua_pushnumber(L, i);
            lua_pushnumber(L, xref_addr);
            lua_settable(L, -3);
            xref_addr = get_next_cref_to(target_addr, xref_addr);
        }

        // Get data xrefs
        xref_addr = get_first_dref_to(target_addr);
        while (xref_addr != BADADDR) {
            i += 1;
            lua_pushnumber(L, i);
            lua_pushnumber(L, xref_addr);
            lua_settable(L, -3);
            xref_addr = get_next_dref_to(target_addr, xref_addr);
        }

        return 1;
    }

    static int c_get_xref(lua_State* L)
    {
        ea_t target_addr = lua_tonumber(L, 1);
        int target_index = lua_tonumber(L, 2);  // Get the desired index

        ea_t xref_addr = get_first_cref_to(target_addr);
        int i = 0;

        while (xref_addr != BADADDR) {
            i += 1;
            if (i == target_index) {
                lua_pushnumber(L, xref_addr);
                return 1;  // Return the specific xref address
            }
            xref_addr = get_next_cref_to(target_addr, xref_addr);
        }

        // If index not found, return nil
        lua_pushnil(L);
        return 1;
    }
}