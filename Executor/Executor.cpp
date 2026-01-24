#include "Executor.h"

#include "Libraries/print.hpp"
#include "Libraries/hexrays.hpp"
#include "Libraries/functions.hpp"
#include "Libraries/xrefs.hpp"
#include "Libraries/strings.hpp"
#include "Libraries/patching.hpp"
#include "Libraries/assembler.hpp"

Executor::Executor()
{

}

Executor::~Executor()
{
	lua_close(this->L);
}

#define lua_register_alias(L, g, ng) lua_getglobal(L, g); \
	lua_setglobal(L, ng)

#define LUA_REGISTER_TABLE_FUNC(L, table, name, func) \
    lua_getglobal(L, table); \
    if (lua_isnil(L, -1)) { \
        lua_pop(L, 1); \
        lua_newtable(L); \
        lua_setglobal(L, table); \
        lua_getglobal(L, table); \
    } \
    lua_pushcfunction(L, func); \
    lua_setfield(L, -2, name); \
    lua_pop(L, 1)

bool Executor::run_script(const std::string& script)
{
	//luda::SendOutput("Received script (" + std::to_string(script.length()) + " chars)");
	int result = luaL_loadbuffer(L, script.c_str(), script.size(), "script");
	if (result != 0) {
		const char* error_msg = lua_tostring(L, -1);
		msg("[ERROR] %s\n", error_msg);
		luda::SendError(error_msg);
		lua_pop(L, 1);  // Pop the error message
		return false;
	}
	int pres = lua_pcall(L, 0, 0, 0);
	if (pres != 0) {
		const char* error_msg = lua_tostring(L, -1);
		msg("[RUNTIME ERROR] %s\n", error_msg);
		luda::SendError(error_msg);
		lua_pop(L, 1);  // Pop the error message
		return false;
	}
	luda::SendSuccess();
	//msg("Executed script successfully!\n");
	return true;
}
bool Executor::initialize()
{
	L = luaL_newstate(); // Create new Lua state
	msg("[Executor] Lua state created: %p\n", L);

	if (L == nullptr) {
		return false; // Failed to create Lua state
	}

	luaL_openlibs(L); // Load standard Lua libraries

	/* Register custom environment */

	// UI related / output
	lua_register(this->L, "print", (lua_CFunction)LUDA::Library::c_print);

	// pseudocode/disassembly related
	LUA_REGISTER_TABLE_FUNC(this->L, "hexrays", "decompile", (lua_CFunction)LUDA::Library::c_decompile);
	LUA_REGISTER_TABLE_FUNC(this->L, "hexrays", "disassemble", (lua_CFunction)LUDA::Library::c_disassemble);

	// functions
	lua_register(this->L, "get_function", (lua_CFunction)LUDA::Library::c_get_func);

	// xrefs
	LUA_REGISTER_TABLE_FUNC(this->L, "xrefs", "get", (lua_CFunction)LUDA::Library::c_get_xrefs);

	// strings
	LUA_REGISTER_TABLE_FUNC(this->L, "strings", "search", (lua_CFunction)LUDA::Library::c_search_strings);

	lua_register(this->L, "hex", (lua_CFunction)LUDA::Library::c_to_hex);

	// patching
	LUA_REGISTER_TABLE_FUNC(this->L, "memory", "write", (lua_CFunction)LUDA::Library::c_patch_bytes);
	LUA_REGISTER_TABLE_FUNC(this->L, "memory", "read", (lua_CFunction)LUDA::Library::c_get_bytes);

	// other shit
	LUA_REGISTER_TABLE_FUNC(this->L, "image", "base", (lua_CFunction)LUDA::Library::c_get_imagebase);
	LUA_REGISTER_TABLE_FUNC(this->L, "image", "first", (lua_CFunction)LUDA::Library::c_get_first_address);
	LUA_REGISTER_TABLE_FUNC(this->L, "image", "last", (lua_CFunction)LUDA::Library::c_get_last_address);


	lua_register(this->L, "assemble", (lua_CFunction)LUDA::Library::c_assemble);

	return true;
}
