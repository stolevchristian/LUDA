#include "ludasocket.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <sstream>

namespace luda {

    // Simple JSON helpers
    namespace json {
        std::string Escape(const std::string& str) {
            std::string result;
            result.reserve(str.size() + 16);
            for (char c : str) {
                switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b";  break;
                case '\f': result += "\\f";  break;
                case '\n': result += "\\n";  break;
                case '\r': result += "\\r";  break;
                case '\t': result += "\\t";  break;
                default:   result += c;      break;
                }
            }
            return result;
        }

        std::string CreateMessage(const std::string& type, const std::string& data) {
            return "{\"type\":\"" + type + "\",\"data\":\"" + Escape(data) + "\"}";
        }

        bool ParseMessage(const std::string& json, std::string& type, std::string& data) {
            auto findValue = [&json](const std::string& key) -> std::string {
                std::string searchKey = "\"" + key + "\":\"";
                size_t pos = json.find(searchKey);
                if (pos == std::string::npos) return "";

                pos += searchKey.length();
                std::string result;
                bool escaped = false;

                for (size_t i = pos; i < json.length(); ++i) {
                    char c = json[i];
                    if (escaped) {
                        switch (c) {
                        case 'n': result += '\n'; break;
                        case 'r': result += '\r'; break;
                        case 't': result += '\t'; break;
                        case 'b': result += '\b'; break;
                        case 'f': result += '\f'; break;
                        default:  result += c;    break;
                        }
                        escaped = false;
                    }
                    else if (c == '\\') {
                        escaped = true;
                    }
                    else if (c == '"') {
                        break;
                    }
                    else {
                        result += c;
                    }
                }
                return result;
                };

            type = findValue("type");
            data = findValue("data");
            if (data.empty()) {
                data = findValue("script");
            }
            return !type.empty();
        }
    }

    // WebSocket frame opcodes
    enum class WsOpcode : uint8_t {
        Continuation = 0x0,
        Text = 0x1,
        Binary = 0x2,
        Close = 0x8,
        Ping = 0x9,
        Pong = 0xA
    };

    class LudaSocket::Impl {
    public:
        Impl() : m_listenSocket(INVALID_SOCKET), m_clientSocket(INVALID_SOCKET),
            m_running(false), m_clientConnected(false) {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
        }

        ~Impl() {
            Stop();
            WSACleanup();
        }

        bool Start(uint16_t port) {
            if (m_running) {
                Stop();
            }

            m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (m_listenSocket == INVALID_SOCKET) {
                return false;
            }

            // Allow address reuse
            int opt = 1;
            setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

            sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port);

            if (bind(m_listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
                closesocket(m_listenSocket);
                m_listenSocket = INVALID_SOCKET;
                return false;
            }

            if (listen(m_listenSocket, 1) == SOCKET_ERROR) {
                closesocket(m_listenSocket);
                m_listenSocket = INVALID_SOCKET;
                return false;
            }

            m_running = true;
            m_acceptThread = std::thread(&Impl::AcceptLoop, this);

            return true;
        }

        void Stop() {
            m_running = false;

            if (m_clientSocket != INVALID_SOCKET) {
                shutdown(m_clientSocket, SD_BOTH);
                closesocket(m_clientSocket);
                m_clientSocket = INVALID_SOCKET;
            }

            if (m_listenSocket != INVALID_SOCKET) {
                closesocket(m_listenSocket);
                m_listenSocket = INVALID_SOCKET;
            }

            if (m_acceptThread.joinable()) {
                m_acceptThread.join();
            }

            if (m_clientThread.joinable()) {
                m_clientThread.join();
            }

            m_clientConnected = false;
        }

        bool IsRunning() const {
            return m_running;
        }

        bool IsClientConnected() const {
            return m_clientConnected;
        }

        void SetScriptCallback(ScriptCallback callback) {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_scriptCallback = callback;
        }

        void SetConnectionCallback(ConnectionCallback callback) {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_connectionCallback = callback;
        }

        void SendMessage(const std::string& type, const std::string& data) {
            if (!m_clientConnected) return;

            std::string message = json::CreateMessage(type, data);
            std::vector<uint8_t> payload(message.begin(), message.end());

            std::lock_guard<std::mutex> lock(m_sendMutex);
            SendFrame(WsOpcode::Text, payload.data(), payload.size());
        }

    private:
        void AcceptLoop() {
            while (m_running) {
                // Set socket to non-blocking for accept with timeout
                fd_set readSet;
                FD_ZERO(&readSet);
                FD_SET(m_listenSocket, &readSet);

                timeval timeout = { 0, 100000 }; // 100ms
                int result = select(0, &readSet, nullptr, nullptr, &timeout);

                if (result > 0 && FD_ISSET(m_listenSocket, &readSet)) {
                    SOCKET clientSocket = accept(m_listenSocket, nullptr, nullptr);
                    if (clientSocket != INVALID_SOCKET) {
                        // Close existing client if any
                        if (m_clientSocket != INVALID_SOCKET) {
                            shutdown(m_clientSocket, SD_BOTH);
                            closesocket(m_clientSocket);
                            if (m_clientThread.joinable()) {
                                m_clientThread.join();
                            }
                        }

                        m_clientSocket = clientSocket;

                        // Perform WebSocket handshake
                        if (PerformHandshake()) {
                            m_clientConnected = true;

                            // Notify callback
                            ConnectionCallback cb;
                            {
                                std::lock_guard<std::mutex> lock(m_callbackMutex);
                                cb = m_connectionCallback;
                            }
                            if (cb) cb(true);

                            // Start client receive thread
                            m_clientThread = std::thread(&Impl::ClientReceiveLoop, this);
                        }
                        else {
                            closesocket(m_clientSocket);
                            m_clientSocket = INVALID_SOCKET;
                        }
                    }
                }
            }
        }

        bool PerformHandshake() {
            char buffer[4096];
            int received = recv(m_clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (received <= 0) return false;
            buffer[received] = '\0';

            std::string request(buffer);

            // Find Sec-WebSocket-Key
            std::string keyHeader = "Sec-WebSocket-Key: ";
            size_t keyPos = request.find(keyHeader);
            if (keyPos == std::string::npos) return false;

            keyPos += keyHeader.length();
            size_t keyEnd = request.find("\r\n", keyPos);
            std::string clientKey = request.substr(keyPos, keyEnd - keyPos);

            // Generate accept key (simplified - proper implementation would use SHA1)
            // For a real implementation, you'd compute: base64(SHA1(clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))
            // Using a simplified version that works with most clients
            std::string acceptKey = ComputeAcceptKey(clientKey);

            // Send handshake response
            std::ostringstream response;
            response << "HTTP/1.1 101 Switching Protocols\r\n";
            response << "Upgrade: websocket\r\n";
            response << "Connection: Upgrade\r\n";
            response << "Sec-WebSocket-Accept: " << acceptKey << "\r\n";
            response << "\r\n";

            std::string responseStr = response.str();
            return send(m_clientSocket, responseStr.c_str(), (int)responseStr.length(), 0) != SOCKET_ERROR;
        }

        std::string ComputeAcceptKey(const std::string& clientKey) {
            // WebSocket GUID
            const std::string guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            std::string combined = clientKey + guid;

            // SHA-1 hash
            uint32_t h0 = 0x67452301;
            uint32_t h1 = 0xEFCDAB89;
            uint32_t h2 = 0x98BADCFE;
            uint32_t h3 = 0x10325476;
            uint32_t h4 = 0xC3D2E1F0;

            // Padding
            std::vector<uint8_t> msg(combined.begin(), combined.end());
            uint64_t originalLen = msg.size() * 8;
            msg.push_back(0x80);
            while ((msg.size() % 64) != 56) {
                msg.push_back(0x00);
            }
            for (int i = 7; i >= 0; --i) {
                msg.push_back((originalLen >> (i * 8)) & 0xFF);
            }

            // Process blocks
            for (size_t i = 0; i < msg.size(); i += 64) {
                uint32_t w[80];
                for (int j = 0; j < 16; ++j) {
                    w[j] = (msg[i + j * 4] << 24) | (msg[i + j * 4 + 1] << 16) |
                        (msg[i + j * 4 + 2] << 8) | msg[i + j * 4 + 3];
                }
                for (int j = 16; j < 80; ++j) {
                    uint32_t temp = w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16];
                    w[j] = (temp << 1) | (temp >> 31);
                }

                uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

                for (int j = 0; j < 80; ++j) {
                    uint32_t f, k;
                    if (j < 20) {
                        f = (b & c) | ((~b) & d);
                        k = 0x5A827999;
                    }
                    else if (j < 40) {
                        f = b ^ c ^ d;
                        k = 0x6ED9EBA1;
                    }
                    else if (j < 60) {
                        f = (b & c) | (b & d) | (c & d);
                        k = 0x8F1BBCDC;
                    }
                    else {
                        f = b ^ c ^ d;
                        k = 0xCA62C1D6;
                    }

                    uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
                    e = d;
                    d = c;
                    c = (b << 30) | (b >> 2);
                    b = a;
                    a = temp;
                }

                h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
            }

            // Create hash bytes
            uint8_t hash[20];
            for (int i = 0; i < 4; ++i) {
                hash[i] = (h0 >> (24 - i * 8)) & 0xFF;
                hash[i + 4] = (h1 >> (24 - i * 8)) & 0xFF;
                hash[i + 8] = (h2 >> (24 - i * 8)) & 0xFF;
                hash[i + 12] = (h3 >> (24 - i * 8)) & 0xFF;
                hash[i + 16] = (h4 >> (24 - i * 8)) & 0xFF;
            }

            // Base64 encode
            return Base64Encode(hash, 20);
        }

        void ClientReceiveLoop() {
            std::vector<uint8_t> buffer(4096);
            std::vector<uint8_t> messageBuffer;

            while (m_running && m_clientConnected) {
                fd_set readSet;
                FD_ZERO(&readSet);
                FD_SET(m_clientSocket, &readSet);

                timeval timeout = { 0, 100000 }; // 100ms
                int result = select(0, &readSet, nullptr, nullptr, &timeout);

                if (result <= 0) continue;

                int received = recv(m_clientSocket, (char*)buffer.data(), (int)buffer.size(), 0);

                if (received <= 0) {
                    // Client disconnected
                    m_clientConnected = false;
                    ConnectionCallback cb;
                    {
                        std::lock_guard<std::mutex> lock(m_callbackMutex);
                        cb = m_connectionCallback;
                    }
                    if (cb) cb(false);
                    break;
                }

                // Parse WebSocket frames
                size_t offset = 0;
                while (offset < static_cast<size_t>(received)) {
                    if (offset + 2 > static_cast<size_t>(received)) break;

                    uint8_t byte0 = buffer[offset];
                    uint8_t byte1 = buffer[offset + 1];

                    bool fin = (byte0 & 0x80) != 0;
                    WsOpcode opcode = static_cast<WsOpcode>(byte0 & 0x0F);
                    bool masked = (byte1 & 0x80) != 0;
                    uint64_t payloadLen = byte1 & 0x7F;

                    offset += 2;

                    if (payloadLen == 126) {
                        if (offset + 2 > static_cast<size_t>(received)) break;
                        payloadLen = (buffer[offset] << 8) | buffer[offset + 1];
                        offset += 2;
                    }
                    else if (payloadLen == 127) {
                        if (offset + 8 > static_cast<size_t>(received)) break;
                        payloadLen = 0;
                        for (int i = 0; i < 8; ++i) {
                            payloadLen = (payloadLen << 8) | buffer[offset + i];
                        }
                        offset += 8;
                    }

                    uint8_t mask[4] = { 0 };
                    if (masked) {
                        if (offset + 4 > static_cast<size_t>(received)) break;
                        memcpy(mask, &buffer[offset], 4);
                        offset += 4;
                    }

                    if (offset + payloadLen > static_cast<size_t>(received)) break;

                    // Extract payload
                    std::vector<uint8_t> payload(payloadLen);
                    for (uint64_t i = 0; i < payloadLen; ++i) {
                        payload[i] = buffer[offset + i];
                        if (masked) {
                            payload[i] ^= mask[i % 4];
                        }
                    }
                    offset += payloadLen;

                    // Handle frame
                    switch (opcode) {
                    case WsOpcode::Text:
                    case WsOpcode::Binary:
                        messageBuffer.insert(messageBuffer.end(), payload.begin(), payload.end());
                        if (fin) {
                            HandleMessage(std::string(messageBuffer.begin(), messageBuffer.end()));
                            messageBuffer.clear();
                        }
                        break;
                    case WsOpcode::Continuation:
                        messageBuffer.insert(messageBuffer.end(), payload.begin(), payload.end());
                        if (fin) {
                            HandleMessage(std::string(messageBuffer.begin(), messageBuffer.end()));
                            messageBuffer.clear();
                        }
                        break;
                    case WsOpcode::Ping:
                        SendFrame(WsOpcode::Pong, payload.data(), payload.size());
                        break;
                    case WsOpcode::Close:
                        m_clientConnected = false;
                        {
                            ConnectionCallback cb;
                            {
                                std::lock_guard<std::mutex> lock(m_callbackMutex);
                                cb = m_connectionCallback;
                            }
                            if (cb) cb(false);
                        }
                        return;
                    default:
                        break;
                    }
                }
            }
        }

        void HandleMessage(const std::string& message) {
            std::string type, data;
            if (json::ParseMessage(message, type, data)) {
                if (type == "execute") {
                    ScriptCallback cb;
                    {
                        std::lock_guard<std::mutex> lock(m_callbackMutex);
                        cb = m_scriptCallback;
                    }
                    if (cb) {
                        cb(data);
                    }
                }
            }
        }

        bool SendFrame(WsOpcode opcode, const uint8_t* payload, size_t payloadLen) {
            if (m_clientSocket == INVALID_SOCKET) return false;

            std::vector<uint8_t> frame;

            // First byte: FIN + opcode
            frame.push_back(0x80 | static_cast<uint8_t>(opcode));

            // Server doesn't mask frames
            if (payloadLen < 126) {
                frame.push_back(static_cast<uint8_t>(payloadLen));
            }
            else if (payloadLen <= 65535) {
                frame.push_back(126);
                frame.push_back(static_cast<uint8_t>((payloadLen >> 8) & 0xFF));
                frame.push_back(static_cast<uint8_t>(payloadLen & 0xFF));
            }
            else {
                frame.push_back(127);
                for (int i = 7; i >= 0; --i) {
                    frame.push_back(static_cast<uint8_t>((payloadLen >> (i * 8)) & 0xFF));
                }
            }

            // Unmasked payload (server to client)
            frame.insert(frame.end(), payload, payload + payloadLen);

            int sent = send(m_clientSocket, (const char*)frame.data(), (int)frame.size(), 0);
            return sent != SOCKET_ERROR;
        }

        std::string Base64Encode(const uint8_t* data, size_t len) {
            static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string result;
            result.reserve(((len + 2) / 3) * 4);

            for (size_t i = 0; i < len; i += 3) {
                uint32_t n = static_cast<uint32_t>(data[i]) << 16;
                if (i + 1 < len) n |= static_cast<uint32_t>(data[i + 1]) << 8;
                if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);

                result += chars[(n >> 18) & 0x3F];
                result += chars[(n >> 12) & 0x3F];
                result += (i + 1 < len) ? chars[(n >> 6) & 0x3F] : '=';
                result += (i + 2 < len) ? chars[n & 0x3F] : '=';
            }

            return result;
        }

    private:
        SOCKET m_listenSocket;
        SOCKET m_clientSocket;
        std::atomic<bool> m_running;
        std::atomic<bool> m_clientConnected;

        std::thread m_acceptThread;
        std::thread m_clientThread;

        std::mutex m_sendMutex;
        std::mutex m_callbackMutex;

        ScriptCallback m_scriptCallback;
        ConnectionCallback m_connectionCallback;
    };

    // LudaSocket implementation
    LudaSocket::LudaSocket() : m_impl(std::make_unique<Impl>()) {}
    LudaSocket::~LudaSocket() = default;

    bool LudaSocket::Start(uint16_t port) {
        return m_impl->Start(port);
    }

    void LudaSocket::Stop() {
        m_impl->Stop();
    }

    bool LudaSocket::IsRunning() const {
        return m_impl->IsRunning();
    }

    bool LudaSocket::IsClientConnected() const {
        return m_impl->IsClientConnected();
    }

    void LudaSocket::SetScriptCallback(ScriptCallback callback) {
        m_impl->SetScriptCallback(callback);
    }

    void LudaSocket::SetConnectionCallback(ConnectionCallback callback) {
        m_impl->SetConnectionCallback(callback);
    }

    void LudaSocket::SendOutput(const std::string& message) {
        m_impl->SendMessage("output", message);
    }

    void LudaSocket::SendError(const std::string& message) {
        m_impl->SendMessage("error", message);
    }

    void LudaSocket::SendSuccess(const std::string& message) {
        m_impl->SendMessage("success", message);
    }

    void LudaSocket::SendPrint(const std::string& message) {
        m_impl->SendMessage("print", message);
    }

    // Global instance
    static LudaSocket* g_instance = nullptr;
    static std::mutex g_instanceMutex;

    LudaSocket& GetInstance() {
        std::lock_guard<std::mutex> lock(g_instanceMutex);
        if (!g_instance) {
            g_instance = new LudaSocket();
        }
        return *g_instance;
    }

    // Convenience functions
    bool Start(uint16_t port) {
        return GetInstance().Start(port);
    }

    void Stop() {
        GetInstance().Stop();
    }

    bool IsRunning() {
        return GetInstance().IsRunning();
    }

    bool IsClientConnected() {
        return GetInstance().IsClientConnected();
    }

    void SetScriptCallback(ScriptCallback callback) {
        GetInstance().SetScriptCallback(callback);
    }

    void SetConnectionCallback(ConnectionCallback callback) {
        GetInstance().SetConnectionCallback(callback);
    }

    void SendOutput(const std::string& message) {
        GetInstance().SendOutput(message);
    }

    void SendError(const std::string& message) {
        GetInstance().SendError(message);
    }

    void SendSuccess(const std::string& message) {
        GetInstance().SendSuccess(message);
    }

    void SendPrint(const std::string& message) {
        GetInstance().SendPrint(message);
    }

} // namespace luda
