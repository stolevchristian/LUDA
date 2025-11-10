#include "Executor.h"

#include "Libraries/print.hpp"
#include "Libraries/hexrays.hpp"
#include "Libraries/functions.hpp"
#include "Libraries/xrefs.hpp"
#include "Libraries/strings.hpp"

Executor::Executor()
{

}

Executor::~Executor()
{
	lua_close(this->L);
}

#define lua_register_alias(L, g, ng) lua_getglobal(L, g); \
	lua_setglobal(L, ng)

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
	lua_register(this->L, "decompile", (lua_CFunction)LUDA::Library::c_decompile);
	lua_register(this->L, "disassemble", (lua_CFunction)LUDA::Library::c_disassemble);

	// functions
	lua_register(this->L, "get_function", (lua_CFunction)LUDA::Library::c_disassemble);
	lua_register_alias(this->L, "get_function", "getfunction");
	lua_register_alias(this->L, "get_function", "get_func");
	lua_register_alias(this->L, "get_function", "getfunc");

	// xrefs

	lua_register(this->L, "get_xrefs", (lua_CFunction)LUDA::Library::c_get_xrefs);
	lua_register_alias(this->L, "get_xrefs", "getxrefs");

	lua_register(this->L, "get_xref", (lua_CFunction)LUDA::Library::c_get_xref);
	lua_register_alias(this->L, "get_xref", "getxref");

	// strings
	lua_register(this->L, "stringsearch", (lua_CFunction)LUDA::Library::c_search_strings);
	lua_register_alias(this->L, "stringsearch", "string_search");
	lua_register_alias(this->L, "stringsearch", "strsearch");
	lua_register_alias(this->L, "stringsearch", "strlookup");
	lua_register_alias(this->L, "stringsearch", "lookupstr");

	lua_register(this->L, "to_hex", (lua_CFunction)LUDA::Library::c_to_hex);
	lua_register_alias(this->L, "to_hex", "tohex");
	lua_register_alias(this->L, "to_hex", "hex");

	std::string print_script = R"(
local pp = {
	print = print,
	indentation = "\t",
	-- list of keys to ignore when printing tables
	ignore = nil,
}

-- keep track of printed tables to avoid infinite loops
-- when tables refer to each other
local printed_tables = {}

local current_indentation = ""

--- Check if a name is ignore
-- Ignored names are configured in pp.ignore
-- @param name
-- @return true if name exists in table of ignored names
local function is_ignored(name)
	if not pp.ignore then
		return false
	end
	for _,ignored in pairs(pp.ignore) do
		if name == ignored then
			return true
		end
	end
	return false
end

local function indent_more()
	current_indentation = current_indentation .. pp.indentation
end

local function indent_less()
	current_indentation = current_indentation:sub(1, #current_indentation - #pp.indentation)
end

--- Format a table into human readable output
-- For every line in the human readable output a callback will be invoked. If no
-- callback is specified print() will be used.
-- @param value The value to convert into a human readable format
-- @param callback Function to call for each line of human readable output
function pp.format_table(value, callback)
	callback = callback or pp.print

	callback(current_indentation .. "{")
	indent_more()
	for name,data in pairs(value) do
		if not is_ignored(name) then
			if type(name) == "string" then
				name = '"'..name..'"'
			else
				name = tostring(name)
			end
			local dt = type(data)
			if dt == "table" then
				callback(current_indentation .. name .. " = [".. tostring(data) .. "]")
				if not printed_tables[data] then
					printed_tables[data] = true
					pp.format_table(data, callback)
				end
			elseif dt == "string" then
				callback(current_indentation .. name .. ' = "' .. tostring(data) .. '"')
			else
				callback(current_indentation .. name .. " = " .. tostring(data))
			end
		end
	end
	indent_less()
	callback(current_indentation .. "}")
end

--- Convert value to a human readable string. If the value to convert is a table
-- it will be formatted using format_table() and returned as a string
-- @param value
-- @return String representation of value
function pp.tostring(value)
	if type(value) ~= "table" then
		return tostring(value)
	end
	local s = ""
	printed_tables = {}
	current_indentation = ""
	pp.format_table(value, function(line)
		s = s .. line .. "\n"
	end)
	return s
end

local function improved_print(...)
	-- iterate through each of the arguments and print them one by one
	local args = { ... }
	table.remove(args, 1)
	local s = ""
	for _, v in pairs(args) do
		s = s .. pp.tostring(v) .. "\t"
	end
	pp.print(s)
end

_G.print = setmetatable(pp, {
	__call = improved_print,
})
	)";
	this->run_script(print_script);
	return true;
}

bool Executor::run_script(std::string& script)
{
	int result = luaL_loadbuffer(L, script.c_str(), script.size(), "script");
	if (result != 0) {
		const char* error_msg = lua_tostring(L, -1);
		msg("[ERROR] %s\n", error_msg);
		lua_pop(L, 1);  // Pop the error message
		return false;
	}
	int pres = lua_pcall(L, 0, 0, 0);
	if (pres != 0) {
		const char* error_msg = lua_tostring(L, -1);
		msg("[RUNTIME ERROR] %s\n", error_msg);
		lua_pop(L, 1);  // Pop the error message
		return false;
	}
	msg("Executed script successfully!\n");
	return true;
}
