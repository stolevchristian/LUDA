#include "../Executor.h"

namespace LUDA::Library
{
    static int c_get_func(lua_State* L)
    {
        if (lua_type(L, 1) == LUA_TNUMBER) /* If address */
        {
            lua_Integer addr = lua_tointeger(L, 1);  // Use lua_tointeger instead
            func_t* func = get_func(addr);

            if (func == NULL) {
                lua_pushnil(L);
                return 1;
            }

            /* push the function addr to lua stack and return */
            lua_pushinteger(L, func->start_ea);
            return 1;
        }
        else if (lua_type(L, 1) == LUA_TSTRING) /* If name */
        {
            const char* func_name = lua_tostring(L, 1);
            ea_t ea = get_name_ea(BADADDR, func_name);

            if (ea == BADADDR) {
                lua_pushnil(L);
                return 1;
            }

            func_t* func = get_func(ea);
            if (func == NULL) {
                lua_pushnil(L);
                return 1;
            }

            lua_pushinteger(L, func->start_ea);
            return 1;
        }
        lua_pushnil(L);
        return 1;
    }
}