#pragma once

#include <string>
#include <functional>
#include <memory>

/*#ifdef LUDASOCKET_EXPORTS
#define LUDA_API __declspec(dllexport)
#else
#define LUDA_API __declspec(dllimport)
#endif
*/
namespace luda {

    // Callback type for when a script is received for execution
    using ScriptCallback = std::function<void(const std::string& script)>;

    // Callback for connection state changes
    using ConnectionCallback = std::function<void(bool connected)>;

    class LudaSocket {
    public:
        LudaSocket();
        ~LudaSocket();

        // Prevent copying
        LudaSocket(const LudaSocket&) = delete;
        LudaSocket& operator=(const LudaSocket&) = delete;

        // Start the WebSocket server on the specified port
        bool Start(uint16_t port = 8080);

        // Stop the server
        void Stop();

        // Check if server is running
        bool IsRunning() const;

        // Check if a client (UI) is connected
        bool IsClientConnected() const;

        // Set callback for incoming script execution requests
        void SetScriptCallback(ScriptCallback callback);

        // Set callback for connection state changes
        void SetConnectionCallback(ConnectionCallback callback);

        // Send responses back to the UI
        void SendOutput(const std::string& message);
        void SendError(const std::string& message);
        void SendSuccess(const std::string& message = "Script executed successfully.");
        void SendPrint(const std::string& message);

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

    LudaSocket& GetInstance();

    bool Start(uint16_t port = 8080);
    void Stop();
    bool IsRunning();
    bool IsClientConnected();
    void SetScriptCallback(ScriptCallback callback);
    void SetConnectionCallback(ConnectionCallback callback);
    void SendOutput(const std::string& message);
    void SendError(const std::string& message);
    void SendSuccess(const std::string& message = "Script executed successfully.");
    void SendPrint(const std::string& message);

} // namespace luda
