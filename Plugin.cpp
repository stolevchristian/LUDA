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

// Define the plugin entry point
plugmod_t* idaapi init() {
    executor = new Executor();

    luda::SetScriptCallback([](const std::string& script) {
        executor->run_script(script);
    });

    // Optional: connection state callback
    luda::SetConnectionCallback([](bool connected) {
        if (connected) {
            msg("[LUDA] UI connected\n");
        }
        else {
            msg("[LUDA] UI disconnected!\n");
        }
        });

    // Start the WebSocket server on port 8080
    if (luda::Start(8080)) {
        msg("[LUDA] Server started on port 8080\n");
        executor->initialize();
    }
    else {
        msg("[LUDA] Failed to start server\n");
    }

    return PLUGIN_OK;
}

void idaapi term() {
    // for some reason crashes startup
    //delete[] executor;
}

bool idaapi run(size_t arg) {
    msg("Plugin started!\n");
    return true;
}

__declspec(dllexport) plugin_t PLUGIN = {
    IDP_INTERFACE_VERSION,
    PLUGIN_PROC,         // Plugin flags
    init,               // Initialization function
    term,               // Cleanup function
    run,                // Main function
    "Launch the LUDA WS server",
    ":shrug:",
    "LUDA",
    "Ctrl-Alt-L" // Example: "Ctrl-Alt-S"
};