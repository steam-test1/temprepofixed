// TODO split this out or something idk
extern "C" {
#include <dlfcn.h>
#include <dirent.h>
}

#include <iostream>
#include <list>
#include <string>
#include <subhook.h>

#if defined(BLT_USING_LIBCXX) // not used otherwise, no point in wasting compile time :p
#   include <vector>
#endif

#include <lua.h>
#include <platform.h>

#include <dsl/LuaInterface.hh>
#include <dsl/FileSystem.hh>

#include <blt/error.hh>
#include <blt/log.hh>
#include <blt/assets.hh>

#include <InitState.h>
#include <lua_functions.h>
#include <util/util.h>

/**
 * Shorthand to reinstall a hook when a function exits, like a trampoline
 */
#define hook_remove(hookName) subhook::ScopedHookRemove _sh_remove_raii(&hookName)

namespace blt {

    using std::cerr;
    using std::cout;
    using std::string;
    using subhook::Hook;
    using subhook::HookOption64BitOffset;

    void* (*dsl_lua_newstate) (dsl::LuaInterface* /* this */, bool, bool, dsl::LuaInterface::Allocation);
    void* (*do_game_update)   (void* /* this */);

    /*
     * Detour Impl
     */

    Hook     gameUpdateDetour;
    Hook     luaNewStateDetour;
    Hook     luaCallDetour;
    Hook     luaCloseDetour;

    void*
    dt_Application_update(void* parentThis)
    {
        hook_remove(gameUpdateDetour);
        return do_game_update(parentThis);
    }

    void
    dt_lua_call(lua_state* state, int argCount, int resultCount)
    {
        //hook_remove(luaNewStateDetour);

        /*
         * For lua_call, we want to give pcall a custom error handler.
         * This gets run before the stack is unwound, so it can print out
         * a stack trace.
         */

        const int target = 1; // Push our handler to the start of the stack

        // Get the value onto the stack, as pcall can't accept indexes
        lua_getref(state, error::check_callback(state));
        lua_insert(state, target);

        // Run the function, and any errors are handled for us.
        lua_pcall(state, argCount, resultCount, target);

        // Done with our error handler
        lua_remove(state, target);
    }

    void*
    dt_dsl_lua_newstate(dsl::LuaInterface* _this, bool b1, bool b2, dsl::LuaInterface::Allocation allocator)
    {
        log::log("removing hook: " + std::to_string(*(uint64_t*) dsl_lua_newstate), log::LOG_INFO);
        hook_remove(luaNewStateDetour);

        log::log("after hook: " + std::to_string(*(uint64_t*) dsl_lua_newstate), log::LOG_INFO);

        void* returnVal = _this->newstate(b1, b2, allocator);

        log::log("Got state", log::LOG_INFO);

        lua_state* state = _this->state;

        if (!state)
        {
            return returnVal;
        }

        blt::lua_functions::initiate_lua(state);

        return returnVal;
    }

    void
    dt_lua_close(lua_state* state)
    {
        blt::lua_functions::close(state);
        hook_remove(luaCloseDetour);
        lua_close(state);
    }

#if defined(BLT_USING_LIBCXX)

    /**
     * uber-simple and highly effective mod_overrides fix
     * Requires libcxx for implementation-level compatibility with PAYDAY
     */

    Hook     sh_dsl_dfs_list_all;
    void        (*dsl_dfs_list_all)(void*, std::vector<std::string>*, std::vector<std::string>*, 
                                    std::string const*);

    // --------------------------
#endif

    namespace platform {
        void InitPlatform() {}
        void ClosePlatform() {}

        idstring *last_loaded_ext = NULL, *last_loaded_name = NULL;
    }

    void
    blt_init_hooks(void* dlHandle)
    {
#define setcall(symbol,ptr) *(void**) (&ptr) = dlsym(dlHandle, #symbol);
        log::log("finding lua functions", log::LOG_INFO);

        /*
         * XXX Still using the ld to get member function bodies from memory, since pedantic compilers refuse to allow 
         * XXX non-static instanceless member function references 
         * XXX (e.g. clang won't allow a straight pointer to _ZN3dsl12LuaInterface8newstateEbbNS0_10AllocationE via `&dsl::LuaInterface::newstate`)
         */

        {
            // _ZN3dsl12LuaInterface8newstateEbbNS0_10AllocationE = dsl::LuaInterface::newstate(...) 
            setcall(_ZN3dsl12LuaInterface8newstateEbbNS0_10AllocationE, dsl_lua_newstate);

            // _ZN11Application6updateEv = Application::update()
            setcall(_ZN11Application6updateEv, do_game_update);
        }

        PD2HOOK_LOG_LOG("installing hooks");

        /*
         * Intercept Init
         */

        {
        log::log("a: " + std::to_string(*(uint64_t*) dsl_lua_newstate), log::LOG_INFO);
            //gameUpdateDetour.Install    ((void*) do_game_update,                (void*) dt_Application_update, HookOption64BitOffset);
            luaNewStateDetour.Install   ((void*) dsl_lua_newstate,              (void*) dt_dsl_lua_newstate, HookOption64BitOffset);
            //luaCloseDetour.Install      ((void*) &lua_close,                    (void*) dt_lua_close, HookOption64BitOffset);
            //luaCallDetour.Install       ((void*) &lua_call,                     (void*) dt_lua_call, HookOption64BitOffset);
        log::log("b: " + std::to_string(*(uint64_t*) dsl_lua_newstate), log::LOG_INFO);
        }

        init_asset_hook(dlHandle);

        pd2hook::InitiateStates(); // TODO move this into the blt namespace
#undef setcall
    }

}

/* vim: set ts=4 softtabstop=0 sw=4 expandtab: */
