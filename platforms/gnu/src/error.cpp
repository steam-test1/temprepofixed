#include "blt/error.hh"

#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <iomanip>

#include <signal.h>
#include <execinfo.h>
#include <cxxabi.h>

#include "blt/log.hh"

namespace blt
{
	namespace error
	{
		using std::string;
		using std::endl;

		// Based off the debug.traceback function
		static void traceback (lua_state *L, void (print)(string))
		{
			lua_Debug ar;

			int level = 1;

			print("stack traceback:");
			std::stringstream buff;
			while (lua_getstack(L, level++, &ar))
			{
				buff << "\t";

				lua_getinfo(L, "Snl", &ar);
				buff << ar.short_src << ":";

				if (ar.currentline > 0)
					buff << ar.currentline << ":";

				if (*ar.namewhat != '\0')  /* is there a name? */
					buff << " in function '" << ar.name << "'";
				else
				{
					if (*ar.what == 'm')  /* main? */
						buff << " in main chunk";
					else if (*ar.what == 'C' || *ar.what == 't')
						buff << " ?";  /* C function or tail call */
					else
						buff << " in function <" << ar.short_src << ":" << ar.linedefined << ">";
				}

				print(buff.str());
				buff.str("");
			}
		}

		static void errlog(string str)
		{
			log::log(str, log::LOG_ERROR);
		}

		std::ostream* crashstream;
		static void crashlog(string str)
		{
			*crashstream << str << endl;
		}

		static void produce_error_file(string message)
		{
			std::stringstream crash;
			crash << message << endl;

			void *func_addrs[1024];
			size_t size;
			char **strings;

			size = backtrace(func_addrs, sizeof(func_addrs) / sizeof(void*) );
			strings = backtrace_symbols(func_addrs, size);

			crash << "Obtained " << size << " C++ stack frames:" << endl;

			// Derived from https://panthema.net/2008/0901-stacktrace-demangled/
			size_t length = 1024;
			char *namepad = (char*) malloc(length);
			for (size_t i = 1; i < size; i++)
			{
				//crash << "  " << strings[i] << endl;
				char *begin_name = 0, *begin_offset = 0, *end_offset = 0, *begin_address = 0, *end_address = 0;

				// First thing to do, remove the path from the filename
				// This both makes the logs much smaller and also removes the user's username
				for (char *p = strings[i]; *p; ++p)
				{
					if(*p == '/')
						strings[i] = p + 1;
				}

				// find parentheses and +address offset surrounding the mangled name:
				// ./module(function+0x15c) [0x8048a6d]
				for (char *p = strings[i]; *p; ++p)
				{
					if (*p == '(')
						begin_name = p;
					else if (*p == '+')
						begin_offset = p;
					else if (*p == ')' && begin_offset)
						end_offset = p;
					else if (*p == '[')
						begin_address = p;
					else if (*p == ']' && begin_address)
					{
						end_address = p;
						break;
					}
				}

				if (begin_name && begin_offset && end_offset
				        && begin_name < begin_offset)
				{
					*begin_name++ = '\0';
					*begin_offset++ = '\0';
					*end_offset = '\0';
					*begin_address++ = '\0';
					*end_address = '\0';

					// mangled name is now in [begin_name, begin_offset) and caller
					// offset in [begin_offset, end_offset). now apply
					// __cxa_demangle():

					int status;
					char* ret = abi::__cxa_demangle(begin_name,
					                                namepad, &length, &status);

					char *usable_name = status ? begin_name : namepad;
					crash << std::setw(15) << begin_address << " " << strings[i] << " : " << usable_name << "+" << begin_offset << endl;

					if (status == 0)
					{
						namepad = ret; // use possibly realloc()-ed string
					}
					else
					{
						// demangling failed. Output function name as a C function with
						// no arguments.
					}
				}
				else
				{
					// couldn't parse the line? print the whole line.
					crash << std::setw(16) << " " << strings[i] << endl;
				}
			}
			free(namepad);
			free(strings);

			std::ofstream logfile("mods/logs/crash.txt");
			logfile << crash.str() << endl;
			log::log("Fatal Error, Aborting: " + crash.str(), log::LOG_ERROR);
		}

		static int error(lua_state* L)
		{
			size_t len;

			const char* crash_mode = getenv("BLT_CRASH");
			if(crash_mode == NULL || string(crash_mode) != "CONTINUE")
			{
				std::stringstream info;
				info << "Lua runtime error: " << lua_tolstring(L, 1, &len) << endl;
				info << endl;
				crashstream = &info;
				traceback(L, crashlog);
				produce_error_file(info.str());

				exit(1); // Does not return
			}

			log::log("lua_call: error in lua_pcall: " + string(lua_tolstring(L, 1, &len)), log::LOG_ERROR);
			traceback(L, errlog);
			log::log("End of stack trace\n", log::LOG_ERROR);

			return 0;
		}


		void push_callback(lua_state* state)
		{
			const char *key = "mods.superblt.err_handler";

			lua_getfield(state, LUA_REGISTRYINDEX, key);
			if(!lua_isnil(state, -1)) {
				return;
			}

			lua_pop(state, 1); // remove the nil

			lua_pushcclosure(state, &error, 0);
			lua_pushvalue(state, -1);
			lua_setfield(state, LUA_REGISTRYINDEX, key);
		}

		void handler(int sig)
		{
			std::stringstream info;
			info << "Segmentation fault!" << endl;
			produce_error_file(info.str());
			abort();
		}

		void myterminate ()
		{
			std::stringstream info;
			info << "Uncaught C++ exception!" << endl;
			produce_error_file(info.str());
			abort();
		}

		void set_global_handlers()
		{
			signal(SIGSEGV, handler);
			std::set_terminate( myterminate );
		}

	}
}

/* vim: set ts=4 softtabstop=0 sw=4 expandtab: */

