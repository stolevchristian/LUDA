#define _SILENCE_CXX20_IS_POD_DEPRECATION_WARNING

#include <windows.h>
#include <cstdio>
#include <iostream>

#define __EA64__
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <xref.hpp>
#include <hexrays.hpp>
#include <ua.hpp>
#include <lines.hpp>

#define IDC_EDIT 1001
#define IDC_RUN_BTN 1002
#define IDC_CLOSE_BTN 1003
#include "Executor/Executor.h"
Executor* executor;


#include <funcs.hpp>


// Define the plugin entry point
plugmod_t* idaapi init() {
	executor = new Executor();
	executor->initialize();
    return PLUGIN_OK;
}

void idaapi term() {
    // for some reason crashes startup
    //delete[] executor;
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
            "LUDA - Script Editor",
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

        //msg("Executing script:\n%s\n", script_text);
		std::string script = script_text;
		executor->run_script(script);
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
    "LUDA - Interface",
    "Wanted_Hotkey" // Example: "Ctrl-Alt-S"
};