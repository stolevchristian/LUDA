#pragma once
#define __EA64__
#include <string>
#include <ida.hpp>
#include <kernwin.hpp>
#include <funcs.hpp>
#include <hexrays.hpp>
#include "../LudaSocket/ludasocket.h"

extern "C" {
#include <Lua/lua.h>
#include <Lua/lauxlib.h>
#include <Lua/lualib.h>
}

class Executor
{
public:
	Executor();
	~Executor();
	bool initialize(); // Create luaState and load standard libraries
	bool run_script(const std::string& script);
private:
	lua_State* L;
};

