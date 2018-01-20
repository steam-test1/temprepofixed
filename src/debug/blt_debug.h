#pragma once

// Enable debug when on windows, until TODO it's ported over
#ifdef _WIN32
#define ENABLE_DEBUG
#endif

#ifdef ENABLE_DEBUG

#include <string>
#include <vector>

namespace pd2hook {
	class DebugConnection {
	public:
		static bool IsLoaded();
		static void Initialize();
		static void Update(void* state);
		static void AddGlobals(void* state);
		static void Log(std::string message);

		void SendLuaMessage(std::string message);
		void WaitForMessage();
	private:
		DebugConnection(std::string host, int port, std::string key);
		~DebugConnection();

		void ReadMessage(int min);
		bool ProcessMessageQueue();

		static void ConnectFromParameters(std::string params);

		static void Connect(std::string host, int port, std::string key);

		// UINT_PTR is SOCKET and we don't want to import winsock2 as it messes it up for others importing this header.
		uintptr_t sock = (uintptr_t)(~0);

		// Store for read bytes if they don't form a complete message
		std::vector<char> buffer;

		// Messages waiting to be sent to Lua
		std::vector<std::string> luaMessageQueue;
	};
}

#endif
