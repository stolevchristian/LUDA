#pragma once
#include <string>
#include <ida.hpp>
#include <kernwin.hpp>
#include <funcs.hpp>
#include <hexrays.hpp>

extern "C" {
#include "../Lua/lapi.h"
#include "../Lua/lua.h"
#include "../Lua/lauxlib.h"
#include "../Lua/lualib.h"
}

class Executor
{
public:
	Executor();
	~Executor();
	bool initialize(); // Create luaState and load standard libraries
	bool run_script(std::string& script);
private:
	lua_State* L;
};

