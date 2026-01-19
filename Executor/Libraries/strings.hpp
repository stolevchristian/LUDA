#include "../Executor.h"

namespace LUDA::Library
{
    static int c_to_hex(lua_State* L)
    {
        lua_Number num = lua_tonumber(L, 1);

        lua_getglobal(L, "string");
        lua_getfield(L, -1, "format");

        lua_pushstring(L, "%X");
        lua_pushnumber(L, num);

        lua_pcall(L, 2, 1, 0);

        lua_remove(L, -2);

        return 1;
    }

    static int c_search_strings(lua_State* L)
    {
        const char* search_string = lua_tostring(L, 1);
        bool exact_match = lua_toboolean(L, 2);
    
        lua_newtable(L);  // Create result table
        int result_index = 0;
    
        // Iterate through all strings in the database
        for (ea_t addr = get_segm_base(getseg(0)); addr != BADADDR; addr = next_head(addr, BADADDR)) {
            int32 str_type = get_str_type(addr);
            if (str_type == STRTYPE_C) {  // Only process C strings
                size_t len = get_item_size(addr);
                qstring str_content;
            
                // Get the string contents properly
                ssize_t result = get_strlit_contents(&str_content, addr, len, str_type, nullptr, STRCONV_ESCAPE);
                if (result == -1) continue;  // Failed to get string
            
                bool match = false;
                if (exact_match) {
                    match = (str_content == search_string);
                } else {
                    match = (str_content.find(search_string) != qstring::npos);
                }
            
                if (match) {
                    result_index += 1;
                    lua_pushnumber(L, result_index);
                
                    // Create a sub-table with string content and address
                    lua_newtable(L);
                    lua_pushstring(L, "string");
                    lua_pushstring(L, str_content.c_str());
                    lua_settable(L, -3);
                    lua_pushstring(L, "address");
                    lua_pushnumber(L, addr);
                    lua_settable(L, -3);
                
                    lua_settable(L, -3);
                }
            }
        }
    
        return 1;  // Return the table
    }
}