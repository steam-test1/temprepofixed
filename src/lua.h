#pragma once

#ifdef _WIN32
#include "../platform_src/signatures/sigdef.h"
#elif __GNUC__
#include "../platforms/gnu/include/lua.hh"
#else
#error Unknown platform - either _WIN32 or __GNUC__ must be defined
#endif
