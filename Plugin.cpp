#include <windows.h>
#include <iostream>
#include <thread>

#include <LudaSocket/ludasocket.h>

#define __EA64__
#include <IdaSDK/ida.hpp>
#include <IdaSDK/idp.hpp>
#include <IdaSDK/loader.hpp>
#include <IdaSDK/hexrays.hpp>

#include <Executor/Executor.h>

Executor* executor;
bool has_initiated = false;
plugmod_t* idaapi init() {
    if (!has_initiated)
    {
        executor = new Executor();

        luda::SetScriptCallback([](const std::string& script) {
            executor->run_script(script);
        });

        luda::SetConnectionCallback([](bool connected) {
            if (connected) {
                msg("[LUDA] UI connected\n");
            }
            else {
                msg("[LUDA] UI disconnected!\n");
            }
        });

        if (luda::Start(8080)) {
            msg("[LUDA] Server started on port 8080\n");
            executor->initialize();
            has_initiated = true;
        }
        else {
            msg("[LUDA] Failed to start server\n");
        }
    }

    return PLUGIN_OK;
}

void idaapi term() {
    /* todo */
}

__declspec(dllexport) plugin_t PLUGIN = {
    IDP_INTERFACE_VERSION,
    PLUGIN_HIDE,
    init,
    term,
    nullptr,
    "Launch the LUDA WS server",
    ":shrug:",
    "LUDA",
    NULL
};