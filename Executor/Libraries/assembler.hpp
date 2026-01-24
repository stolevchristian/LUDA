#include "../Executor.h"
#include <keystone/keystone.h>

namespace LUDA::Library
{
    int c_assemble(lua_State* L) {
        const char* code = luaL_checkstring(L, 1);

        ks_engine* ks;
        if (ks_open(KS_ARCH_X86, KS_MODE_64, &ks) != KS_ERR_OK) {
            lua_pushnil(L);
            lua_pushstring(L, "Failed to initialize Keystone");
            return 2;
        }

        unsigned char* encoded;
        size_t size;
        size_t count;

        if (ks_asm(ks, code, 0, &encoded, &size, &count) != KS_ERR_OK) {
            lua_pushnil(L);
            lua_pushfstring(L, "Assembly failed: %s", ks_strerror(ks_errno(ks)));
            ks_close(ks);
            return 2;
        }

        lua_newtable(L);
        for (size_t i = 0; i < size; i++) {
            lua_pushinteger(L, encoded[i]);
            lua_rawseti(L, -2, i + 1);
        }

        ks_free(encoded);
        ks_close(ks);

        return 1;
    }
}