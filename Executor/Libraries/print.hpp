#include "../Executor.h"

namespace LUDA::Library
{
    lua_CFunction c_print(lua_State* L)
    {
        int nargs = lua_gettop(L);
        std::string result;

        for (int i = 1; i <= nargs; i++) {
            if (i > 1) result += "\n";

            if (lua_isstring(L, i)) {
                result += lua_tostring(L, i);
            }
            else if (lua_isboolean(L, i)) {
                result += lua_toboolean(L, i) ? "true" : "false";
            }
            else if (lua_isnil(L, i)) {
                result += "nil";
            }
            else if (lua_isnumber(L, i)) {
                result += "0x" + std::to_string((ea_t)lua_tonumber(L, i));
            }
            else if (lua_istable(L, i)) {
                result += "table";
            }
            else if (lua_isfunction(L, i)) {
                result += "function";
            }
            else if (lua_isuserdata(L, i)) {
                result += "userdata";
            }
            else {
                result += lua_typename(L, lua_type(L, i));
            }
        }

        msg("%s\n", result.c_str());
        return 0;
    }
}