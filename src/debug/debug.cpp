#include <winsock2.h>
#include <ws2tcpip.h>
#include <atlstr.h>
#include <sstream>
#include <fstream>
#include <map>

#include "debug.h"
#include "lua.h"
#include "util/util.h"

using std::string;
using std::stringstream;
using std::to_string;

#define DEFAULT_BUFLEN 512
#define WRITE_DATA(stream, data) { \
	uint32_t bigend = htonl(data); \
	stream.write(reinterpret_cast<const char *>(&bigend), sizeof(bigend)); \
}

enum MessageType {
	MSGT_Log = 1,
	MSGT_Init,
	MCGT_Continue,
	MCGT_StartVR, // no longer used by the IDE, still supported however
	MCGT_LuaMessage,
	MSGT_LuaMesage
};

namespace pd2hook {
	static DebugConnection* connection = NULL;
	static std::map<string, string> parameters;

	static int luaF_isloaded(lua_State* L) {
		lua_pushboolean(L, connection != NULL);
		return 1;
	}

	static int luaF_send_message(lua_State* L) {
		connection->SendLuaMessage(lua_tostring(L, 1));
		return 0;
	}

	static int luaF_wait(lua_State* L) {
		connection->WaitForMessage();
		DebugConnection::Update(L);
		return 0;
	}

	static int luaF_get_param(lua_State* L) {
		string name = lua_tostring(L, 1);
		if (parameters.count(name)) {
			lua_pushstring(L, parameters[name].c_str());
		}
		else {
			// TODO add lua_pushnil signature, and use that instead
			lua_pushboolean(L, false);
		}
		return 1;
	}

	static void startVR() {
		char exe[] = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\PAYDAY 2\\payday2_win32_release_vr.exe";
		LPSTR args = GetCommandLineA(); // Leave the original EXE name in there. PD2 doesn't seem to care. Was: "payday2_win32_release_vr.exe";
		PD2HOOK_LOG_LOG("Exec: Starting " + string(exe));
		Sleep(1000);

		// additional information
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;

		// set the size of the structures
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// start the program up
		CreateProcessA
		(
			exe,   // the path
			args,                // Command line
			NULL,                   // Process handle not inheritable
			NULL,                   // Thread handle not inheritable
			FALSE,                  // Set handle inheritance to FALSE
			CREATE_NEW_CONSOLE,     // Opens file in a separate console
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi           // Pointer to PROCESS_INFORMATION structure
		);
		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		// Quit now
		ExitProcess(0);
	}

	bool DebugConnection::IsLoaded() {
		return connection != NULL;
	}

	void DebugConnection::Initialize() {
		LPWSTR *szArglist;
		int nArgs;
		int i;

		PD2HOOK_LOG_LOG("Command line: " + string(GetCommandLineA()));

		szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
		if (NULL == szArglist) {
			PD2HOOK_LOG_WARN("Debug: Could not read command line!");
			return;
		}

		string host;
		int port;
		string key;

		for (i = 0; i < nArgs; i++) {
			// Really, Windows? Can't we just use UTF-8 and be done already?
			if (!lstrcmpW(L"--debug-params", szArglist[i])) {
				if (i == nArgs - 1) {
					PD2HOOK_LOG_WARN("Debug: --debug-params supplied as last parameter!");
					break;
				}

				string params = CW2A(szArglist[i + 1]);
				ConnectFromParameters(params);

				// PD2HOOK_LOG_LOG("debug params: '" + host + "' . '" + port + "' . '" + key + "'");
			}
			else if (!StrCmpW(L"--debug-lua-param", szArglist[i])) {
				if (i >= nArgs - 2) {
					PD2HOOK_LOG_WARN("Debug: --debug-lua-param requires two further arguments!");
					break;
				}

				string name = CW2A(szArglist[i + 1]);
				string value = CW2A(szArglist[i + 2]);
				parameters[name] = value;
			}
		}

		const string debugfile_name = "remote-debug.conf";
		std::ifstream infile(debugfile_name);
		if (infile.good()) {
			string line;
			getline(infile, line);
			infile.close();

			DeleteFileA(debugfile_name.c_str());

			ConnectFromParameters(line);

			// TODO lua parameters
		}

		// Free memory allocated for CommandLineToArgvW arguments.
		LocalFree(szArglist);
	}

	void DebugConnection::Update(void* state) {
		if (!connection) return;
		lua_State* L = (lua_State*)state;

		// Read any new commands
		connection->ReadMessage(0);

		auto &mq = connection->luaMessageQueue;
		if (!mq.empty()) {
			lua_getglobal(L, "blt_debugger");
			lua_getfield(L, -1, "lua_msg_callback");

			// If there's anything to listen for messages, process them
			if (lua_isfunction(L, -1)) {
				// Add each available message to a table
				int num = mq.size();
				lua_createtable(L, num, 0);
				for (size_t i = 0; i < num; i++) {
					string &msg = mq[i];
					lua_pushlstring(L, msg.c_str(), msg.length());
					lua_rawseti(L, -2, i + 1);
				}

				// Clear the buffer *before* we call the function
				// Otherwise if the state changes, we can send the same message calls every update
				mq.clear();

				// Call the function, popping it and it's argument off
				lua_call(L, 1, 0);
			}
			else {
				PD2HOOK_LOG_WARN("Debug message received, blt_debugger.lua_msg_callback missing");

				// Since we're not calling the function, remove the value ourselves
				lua_remove(L, -1);

				// Clear the buffer
				mq.clear();
			}

			// Remove blt_debugger from the stack
			lua_remove(L, -1);
		}
	}

	void DebugConnection::AddGlobals(void* state) {
		lua_State* L = (lua_State*)state;

		luaL_Reg vrLib[] = {
			{ "is_loaded", luaF_isloaded },
			{ "send_message", luaF_send_message },
			{ "wait", luaF_wait },
			{ "get_parameter", luaF_get_param },
			{ NULL, NULL }
		};
		luaI_openlib(L, "blt_debugger", vrLib, 0);
	}

	void DebugConnection::Log(std::string message) {
		if (!connection) return;

		stringstream data;
		WRITE_DATA(data, MSGT_Log);
		WRITE_DATA(data, message.length());
		data << message;

		string str = data.str();
		send(connection->sock, str.c_str(), str.length(), 0);
		// TODO check result
	}

	void DebugConnection::SendLuaMessage(std::string message) {
		if (!connection) return;

		stringstream data;
		WRITE_DATA(data, MSGT_LuaMesage);
		WRITE_DATA(data, message.length());
		data << message;

		string str = data.str();
		send(connection->sock, str.c_str(), str.length(), 0);
		// TODO check result
	}

	void DebugConnection::WaitForMessage() {
		// Switch to blocking mode
		u_long mode = 0;  // 0 to disable non-blocking socket
		ioctlsocket(sock, FIONBIO, &mode);

		// Wait until the message arrives
		ReadMessage(1);

		// Switch to non-blocking mode
		mode = 1;  // 1 to enable non-blocking socket
		ioctlsocket(sock, FIONBIO, &mode);
	}

	DebugConnection::DebugConnection(string host, int port, string key) {
		PD2HOOK_LOG_LOG("Connecting to " + host + " on " + to_string(port));
		WSADATA wsaData;
		int iResult;

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			throw string("WSAStartup failed with error: ") + to_string(iResult);
		}

		// Create a SOCKET for connecting to server
		sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET) {
			string msg = "socket failed with error: " + WSAGetLastError();
			WSACleanup();
			throw msg;
		}

		struct addrinfo *result = NULL,
			*ptr = NULL,
			hints;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// Resolve the server address and port
		string port_str = to_string(port);
		iResult = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
		if (iResult != 0) {
			WSACleanup();
			throw "getaddrinfo failed with error: %d\n" + to_string(iResult);
		}

		// Attempt to connect to an address until one succeeds
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

			// Create a SOCKET for connecting to server
			sock = socket(ptr->ai_family, ptr->ai_socktype,
				ptr->ai_protocol);
			if (sock == INVALID_SOCKET) {
				string msg = "socket failed with error: " + WSAGetLastError();
				WSACleanup();
				throw msg;
			}

			// Connect to server.
			iResult = connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult == SOCKET_ERROR) {
				closesocket(sock);
				sock = INVALID_SOCKET;
				continue;
			}
			break;
		}

		freeaddrinfo(result);

		if (sock == INVALID_SOCKET) {
			WSACleanup();
			throw string("Unable to connect to server!");
		}

		PD2HOOK_LOG_LOG("Connected to debugging server");

		// Send an initial buffer
		stringstream data;
		WRITE_DATA(data, MSGT_Init);
		WRITE_DATA(data, key.length());
		data << key;

		string str = data.str();
		iResult = send(sock, str.c_str(), str.length(), 0);
		if (iResult == SOCKET_ERROR) {
			string msg = "send failed with error: " + WSAGetLastError();
			closesocket(sock);
			WSACleanup();
			throw msg;
		}

		PD2HOOK_LOG_LOG("Sent Debug AUTH");

		// Process the connection message
		ReadMessage(1);

		// Switch to non-blocking mode
		u_long mode = 1;  // 1 to enable non-blocking socket
		ioctlsocket(sock, FIONBIO, &mode);
	}

	DebugConnection::~DebugConnection() {
		// shutdown the connection since no more data will be sent
		int iResult = shutdown(sock, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			string msg = "shutdown failed with error: " + WSAGetLastError();
			closesocket(sock);
			WSACleanup();
			throw msg;
		}

		// cleanup
		closesocket(sock);
		WSACleanup();
	}

	void DebugConnection::ReadMessage(int min) {
		char recvbuf[DEFAULT_BUFLEN];
		int res;

		int processed = 0;

		// Receive until the peer closes the connection
		do {
			res = recv(sock, recvbuf, DEFAULT_BUFLEN, 0);
			if (res > 0) {
				// TODO do we need to change the put pointer?
				for (auto i = 0; i < res; i++) {
					buffer.push_back(recvbuf[i]);
				}
				//printf("Bytes received: %d into %d\n", res, buffer.size());
			}
			else if (res == 0) {
				printf("Debugging connection closed\n");
				exit(1);
				break;
			}
			else if (WSAGetLastError() == WSAEWOULDBLOCK) {
				// We're in non-blocking mode and ran out of data to process, don't enter an infinite loop
				return;
			}
			else {
				printf("recv failed with error: %d\n", WSAGetLastError());
				break;
			}

			// TODO argument to disable this
			while (ProcessMessageQueue()) {
				processed++;
			}
		} while (res > 0 && (min == 0 || processed < min));
	}

	bool DebugConnection::ProcessMessageQueue() {
		if (buffer.size() == 0)
			return false;

		uint32_t value;
		unsigned int numRead = 0;
		unsigned char readbuff[4];

		// Flip around the indexes to account for endianess
#define INDEX_READ(index) \
if(numRead + 4 > buffer.size()) return false; \
readbuff[0] = buffer[index + 3]; \
readbuff[1] = buffer[index + 2]; \
readbuff[2] = buffer[index + 1]; \
readbuff[3] = buffer[index + 0]; \
numRead += 4; \
value = *(uint32_t*)readbuff

#define READ INDEX_READ(numRead)

#define APPLY \
buffer.erase(buffer.begin(), buffer.begin()+numRead); \
return true

		READ;
		int type = value;

		// std::cout << "Typeinfo: " << value << std::endl;

		switch (type) {
		case MCGT_Continue:
			// Everything's fine
			APPLY;
		case MCGT_StartVR:
			// Relaunch to VR
			startVR();
			APPLY;
		case MCGT_LuaMessage:
			// Read an arbitary string to pass to Lua

			// Find the length of said string
			READ;
			int length = value;

			if (numRead + length > buffer.size()) return false;

			// Read the string
			// TODO Do we need anything special for Unicode handling?
			string str(buffer.begin() + numRead, buffer.begin() + numRead + length);
			numRead += length;

			// Add the string to the queue - they'll be read off and passed to Lua later.
			luaMessageQueue.push_back(str);

			// Remove this message from the queue
			APPLY;
		}

#undef READ
#undef INDEX_READ
#undef APPLY

		//printf("Skipping %d with %d bytes\n", type, buffer.size());

		return false;
	}

	void DebugConnection::ConnectFromParameters(std::string params) {
		auto i = 0;
		auto pos = params.find(":");
		string host = params.substr(0, pos);
		auto npos = params.find(":", pos + 1);
		string port = params.substr(pos + 1, npos - pos - 1);
		string key = params.substr(npos + 1);

		try {
			Connect(host, std::stoi(port), key);
		}
		catch (string msg) {
			PD2HOOK_LOG_ERROR("While initializing debug connection: " + msg);
		}
	}

	void DebugConnection::Connect(string host, int port, string key) {
		connection = new DebugConnection(host, port, key);
	}
}
