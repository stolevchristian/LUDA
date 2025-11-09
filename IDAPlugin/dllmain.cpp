#define _SILENCE_CXX20_IS_POD_DEPRECATION_WARNING

#include <windows.h>
#include <cstdio>
#include <iostream>
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <xref.hpp>
#include <hexrays.hpp>


extern "C" {
#include "Lua/lapi.h"
#include "Lua/lua.h"
#include "Lua/lauxlib.h"
#include "Lua/lualib.h"
}

#define IDC_EDIT 1001
#define IDC_RUN_BTN 1002
#define IDC_CLOSE_BTN 1003

lua_State* L;

void find_all_xrefs_to(ea_t target_addr) {
    /*ea_t xref_addr = get_first_dref_to(target_addr);
    while (xref_addr != BADADDR) {
        msg("Data xref from: 0x%X\n", xref_addr);
        xref_addr = get_next_dref_to(target_addr, xref_addr);
    }*/
    ea_t xref_addr = get_first_cref_to(target_addr);
    while (true)
    {
        if (xref_addr == BADADDR)
            break;

        msg("Code xref from: 0x%X\n", xref_addr);

        func_t* func = get_func(xref_addr);
        hexrays_failure_t hf;

        msg("Func: %llx\n", func);
        if (func == nullptr)
        {
            xref_addr = get_next_cref_to(target_addr, xref_addr);
            continue;
        }
        cfuncptr_t cfunc = decompile(func, &hf, DECOMP_WARNINGS);
        if (cfunc == nullptr) {
            msg("Failed to decompile function at address: 0x%X\n", xref_addr);
            xref_addr = get_next_dref_to(target_addr, xref_addr);
            continue;
        }

        const strvec_t& sv = cfunc->get_pseudocode();
        for (int i = 0; i < sv.size(); i++) {
            qstring buf;
            tag_remove(&buf, sv[i].line);
            msg("%s\n", buf.c_str());
        }
        xref_addr = get_next_cref_to(target_addr, xref_addr);
    }
}

static int c_get_xrefs(lua_State* L)
{
    ea_t target_addr = lua_tonumber(L, 1);
    msg("[C++] Target ADDR: %p\n", target_addr);
    ea_t xref_addr = get_first_cref_to(target_addr);
    lua_newtable(L);
    int i = 0;
    while (xref_addr != BADADDR) {
        i += 1;
        //msg("Code xref from: 0x%X\n", xref_addr);
        lua_pushnumber(L, i);
        lua_pushnumber(L, xref_addr);
        lua_settable(L, -3);
        xref_addr = get_next_cref_to(target_addr, xref_addr);
    }
    return 1;
}

static int c_to_hex(lua_State* L)
{
    // Get the number argument from stack position 1
    lua_Number num = lua_tonumber(L, 1);

    // Get string.format function
    lua_getglobal(L, "string");      // Stack: [string]
    lua_getfield(L, -1, "format");   // Stack: [string, format]

    // Push arguments to string.format
    lua_pushstring(L, "%X");         // Stack: [string, format, "%X"]
    lua_pushnumber(L, num);          // Stack: [string, format, "%X", num]

    // Call string.format("%X", num) with 2 args, 1 result
    lua_pcall(L, 2, 1, 0);           // Stack: [string, "result"]

    // Clean up the string table from stack
    lua_remove(L, -2);               // Stack: ["result"]

    return 1;
}

static int c_print(lua_State* L)
{
    int nargs = lua_gettop(L);  // Get number of arguments
    std::string result;

    for (int i = 1; i <= nargs; i++) {
        if (i > 1) result += "\t";  // Add tab between arguments like Lua's print

        // Convert to string
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
            int num = lua_tonumber(L, i);
            std::string numstr = std::to_string(num);
            result += numstr;
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

    // Do whatever you want with result (print it, log it, etc)
    msg("%s\n", result.c_str());  // IDA's msg() function

    return 0;  // Return 0 values to Lua
}

bool is_addr_in_function(ea_t addr)
{
    func_t* func = get_func(addr);
    return func != nullptr;
}

// Get the function containing the address
func_t* get_function_at(ea_t addr)
{
    return get_func(addr);
}

bool decompile_function(ea_t func_addr, std::string& out_pseudocode)
{
    if (!init_hexrays_plugin()) {
        msg("Hex-Rays decompiler is not available.\n");
        return false;
    }
    func_t* func = get_func(func_addr);
    hexrays_failure_t hf;

    msg("Func: %llx\n", func);
    if (func == nullptr)
    {
        return false;
    }
    cfuncptr_t cfunc = decompile(func, &hf, DECOMP_WARNINGS);
    if (cfunc == nullptr) {
        msg("Failed to decompile function at address: 0x%X\n", func_addr);
        return false;
    }
    const strvec_t& sv = cfunc->get_pseudocode();
    for (int i = 0; i < sv.size(); i++) {
        qstring buf;
        tag_remove(&buf, sv[i].line);
        //msg("%s\n", buf.c_str());
        out_pseudocode += buf.c_str();
        out_pseudocode += "\n";
    }
    //out_pseudocode = "gay";
    return true;
}
// Usage example
static int c_decompile(lua_State* L)
{
    ea_t addr = (ea_t)lua_tonumber(L, 1);

    // Check if address is in a function
    if (!is_addr_in_function(addr)) {
        lua_pushnil(L);
        lua_pushstring(L, "Address is not in a function");
        return 2;
    }
    
    std::string pseudocode = "no pseudo";
    if (decompile_function(addr, pseudocode)) {
        lua_pushstring(L, pseudocode.c_str());
        return 1;
    }
    else {
        lua_pushnil(L);
        lua_pushstring(L, "Decompilation failed");
        return 2;
    }
}

static int c_get_func(lua_State* L)
{
    if (lua_type(L, 1) == LUA_TNUMBER) /* If address */
    {
        DWORD addr = lua_tonumber(L, 1);
        func_t* func = get_func(addr);

        if (func == NULL) {
            lua_pushnil(L);
            return 1;
        }

        /* push the function addr to lua stack and return */
        lua_pushnumber(L, func->start_ea);
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

        lua_pushnumber(L, func->start_ea);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
#define lua_registercfunc(func, name) lua_pushvalue(L, LUA_GLOBALSINDEX); \
    lua_pushcclosure(L, func, 0); \
    lua_setfield(L, -2, name);

// Define the plugin entry point
plugmod_t* idaapi init() {
    msg("Initiating lua state..\n");
	L = luaL_newstate();
    luaL_openlibs(L);

    lua_registercfunc(c_print, "print");
    lua_registercfunc(c_get_xrefs, "get_xrefs");
    lua_registercfunc(c_to_hex, "to_hex");
    lua_registercfunc(c_decompile, "decompile");
    lua_registercfunc(c_get_func, "get_func");

	msg("Lua state:%p\n", L);

    std::cout << "This works" << std::endl;
    return PLUGIN_OK;
}

void idaapi term() {
    lua_close(L);
}

class ScriptEditorWindow {
public:
    HWND hwnd;
    HWND edit_hwnd;

    ScriptEditorWindow() : hwnd(nullptr), edit_hwnd(nullptr) {}

    void create() {
        WNDCLASS wc = {};
        wc.lpfnWndProc = window_proc;
        wc.lpszClassName = "ScriptEditorClass";
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

        RegisterClass(&wc);

        hwnd = CreateWindowEx(
            0,
            "ScriptEditorClass",
            "Script Editor",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
            nullptr, nullptr, nullptr, this
        );

        if (hwnd) {
            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);
        }
    }

    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        ScriptEditorWindow* pThis;

        if (msg == WM_CREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lparam);
            pThis = (ScriptEditorWindow*)pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->hwnd = hwnd;
            pThis->create_controls();
        }
        else {
            pThis = (ScriptEditorWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (!pThis)
            return DefWindowProc(hwnd, msg, wparam, lparam);

        switch (msg) {
        case WM_COMMAND: {
            int control_id = LOWORD(wparam);
            if (control_id == IDC_RUN_BTN) {
                pThis->run_script();
            }
            else if (control_id == IDC_CLOSE_BTN) {
                DestroyWindow(hwnd);
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
        return 0;
    }

    void create_controls() {
        // Multiline textbox
        edit_hwnd = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            "EDIT",
            "",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
            10, 10, 470, 300,
            hwnd, (HMENU)IDC_EDIT, nullptr, nullptr
        );

        // Set font
        HFONT font = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Courier New");
        SendMessage(edit_hwnd, WM_SETFONT, (WPARAM)font, TRUE);

        // Run button
        HWND run_btn = CreateWindowEx(
            0,
            "BUTTON",
            "Run Script",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 320, 100, 30,
            hwnd, (HMENU)IDC_RUN_BTN, nullptr, nullptr
        );

        // Close button
        HWND close_btn = CreateWindowEx(
            0,
            "BUTTON",
            "Close",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            380, 320, 100, 30,
            hwnd, (HMENU)IDC_CLOSE_BTN, nullptr, nullptr
        );
    }

    void run_script() {
        int text_len = GetWindowTextLength(edit_hwnd);
        if (text_len == 0) {
            MessageBox(hwnd, "No script entered", "Error", MB_OK | MB_ICONWARNING);
            return;
        }

        char* script_text = new char[text_len + 1];
        GetWindowText(edit_hwnd, script_text, text_len + 1);

        msg("Executing script:\n%s\n", script_text);
        int result = luaL_loadbuffer(L, script_text, strlen(script_text), "script");

        if (result != 0) {
            const char* error_msg = lua_tostring(L, -1);
            msg("Lua Error: %s\n", error_msg);
            lua_pop(L, 1);  // Pop the error message
            return;
        }
        int pres = lua_pcall(L, 0, 0, 0);
        if (pres != 0) {
            const char* error_msg = lua_tostring(L, -1);
            msg("Runtime Error: %s\n", error_msg);
            lua_pop(L, 1);  // Pop the error message
            return;
        }
        delete[] script_text;
    }
};

void open_script_editor() {
    ScriptEditorWindow* editor = new ScriptEditorWindow();
    editor->create();
}

bool idaapi run(size_t arg) {
    msg("Plugin started!\n");
    open_script_editor();
    return true;
}

__declspec(dllexport) plugin_t PLUGIN = {
    IDP_INTERFACE_VERSION,
    PLUGIN_PROC,         // Plugin flags
    init,               // Initialization function
    term,               // Cleanup function
    run,                // Main function
    "Description",
    "Help",
    "Wanted_Name",
    "Wanted_Hotkey" // Example: "Ctrl-Alt-S"
};