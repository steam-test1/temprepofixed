#pragma once

#include <string>
#include <vector>

namespace pd2hook {
	class DebugConnection {
	public:
		static void Initialize();
		static void Update(void* state);
		static void AddGlobals(void* state);
		static void Log(std::string message);
	private:
		DebugConnection(std::string host, int port, std::string key);
		~DebugConnection();

		void ReadMessage(int min);
		bool ProcessMessageQueue();

		static void DebugConnection::Connect(std::string host, int port, std::string key);

		// UINT_PTR is SOCKET and we don't want to import winsock2 as it messes it up for others importing this header.
		UINT_PTR sock = (UINT_PTR)(~0);

		// Store for read bytes if they don't form a complete message
		std::vector<char> buffer;

		// Messages waiting to be sent to Lua
		std::vector<std::string> luaMessageQueue;
	};
}
