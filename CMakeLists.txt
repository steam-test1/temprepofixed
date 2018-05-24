cmake_minimum_required(VERSION 3.0)
project(SuperBLT)

if(CMAKE_SYSTEM_NAME MATCHES "Linux") # Add other systems, if they get PD2 support
	set(UNIX true)
endif()

###############################################################################
## misc. definitions ##########################################################
###############################################################################

# Use CXX 14
set(CMAKE_CXX_STANDARD 14)

# Enable XAudio
add_definitions(-DENABLE_XAUDIO)

###############################################################################
## file globbing ##############################################################
###############################################################################

# these instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variables
# `sources` and `data`
file(GLOB_RECURSE sources		src/*.cpp src/*.c src/*.h)
file(GLOB_RECURSE win32_sources		platforms/w32/*.cpp platforms/w32/*.h)
set(		  win32_loader_path	platforms/w32/loader)
file(GLOB_RECURSE win32_loaders		${win32_loader_path}/*.cpp)
list(REMOVE_ITEM  win32_sources		${win32_loaders})
file(GLOB_RECURSE gnu_sources		platforms/gnu/src/*.cpp platforms/gnu/include/*.hh)
set(		  gnu_loader_file	${CMAKE_CURRENT_SOURCE_DIR}/platforms/gnu/src/blt_main.cpp)
list(REMOVE_ITEM  gnu_sources		${gnu_loader_file})
set(		  gnu_link_script	${CMAKE_CURRENT_SOURCE_DIR}/platforms/gnu/link.ld)
# you can use set(sources src/main.cpp) etc if you don't want to
# use globing to find files automatically

###############################################################################
## target definitions #########################################################
###############################################################################

file(GLOB_RECURSE mxml_sources		lib/mxml/*.c lib/mxml/*.h)
add_library(mxml STATIC ${mxml_sources})
include_directories(mxml lib/configs/mxml)
if(UNIX)
	target_compile_options(mxml PRIVATE -std=gnu99 -Wall -fPIC)
elseif(WIN32)
	target_compile_options(mxml PRIVATE -D_CRT_SECURE_NO_WARNINGS)
endif()
target_include_directories(mxml PUBLIC lib/mxml)

file(GLOB_RECURSE wren_sources
	lib/wren/src/include/wren.h # Public source file
		lib/wren/src/vm/*.c lib/wren/src/vm/*.h # Core VM files
		lib/wren/src/optional/*.c lib/wren/src/optional/*.h # Optional components (Meta and Random)
)
add_library(wren STATIC ${wren_sources})
include_directories(wren lib/wren/src/optional lib/wren/src/vm)
if(MSVC)
	target_compile_options(wren PRIVATE -D_CRT_SECURE_NO_WARNINGS)
else()
	target_compile_options(wren PRIVATE -std=gnu99 -Wall -fPIC)
endif()
target_include_directories(wren PUBLIC lib/wren/src/include)

add_library(subhook STATIC lib/subhook/subhook.c)
include_directories(subhook lib/subhook)
if(UNIX)
	target_compile_options(subhook PRIVATE -std=gnu99 -Wall -fPIC)
elseif(WIN32)
	target_compile_options(subhook PRIVATE -DSUBHOOK_EXPORT=;)
endif()
target_include_directories(subhook PUBLIC lib/subhook)

###############################################################################
## Base (non-platform-specific) part of SuperBLT ##############################
###############################################################################

if(UNIX)
	add_library(SuperBLT STATIC ${sources} ${gnu_sources})
	include_directories (platforms/gnu/include)

	# Use modern C++, and show all warnings
	target_compile_options(SuperBLT PUBLIC -Wall -Werror -fPIC)

	# Enable this to see missing references:
	#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -z defs")

	# LibCXX / Patches support
	if (USE_LIBCXX)
		message("Using Clang/LibC++")
		add_definitions(-DBLT_USING_LIBCXX)
		set(CMAKE_CXX_COMPILER clang++)
	endif()
elseif(WIN32)
	add_library(SuperBLT STATIC ${sources} ${win32_sources})
	target_compile_options(SuperBLT PRIVATE
		-DAL_LIBTYPE_STATIC
		-DCURL_STATICLIB
		-DSUBHOOK_STATIC
	)

	# Link against libraries
	target_link_libraries(SuperBLT winmm.lib)
	target_link_libraries(SuperBLT ws2_32.lib)
	target_link_libraries(SuperBLT crypt32.lib)
else()
	# TODO raise error
endif()

# Allow both SuperBLT and the loaders access to the headers
target_include_directories(SuperBLT PUBLIC src)

###############################################################################
## dependencies ###############################################################
###############################################################################

target_link_libraries(SuperBLT mxml)
target_link_libraries(SuperBLT wren)
target_link_libraries(SuperBLT subhook)

if(WIN32)
	set(ZLIB_ROOT $ENV{ZLIB_ROOT} $ENV{ZLIB_ROOT}/Release)
	set(ZLIB_LIBRARY $ENV{ZLIB_ROOT}/Release/zlibstatic.lib) # Force the static version of ZLib
	set(CURL_INCLUDE_DIR $ENV{CURL_INCLUDE_DIR})
	set(CURL_LIBRARY $ENV{CURL_LIBRARY})
	set(OPENSSL_ROOT_DIR $ENV{OPENSSL_ROOT_DIR})
	set(OPENAL_LIBRARY $ENV{OpenALSoft_ROOT}/build/Release/OpenAL32.lib)
	set(OPENAL_INCLUDE_DIR $ENV{OpenALSoft_ROOT}/include)

	# Use static libs on Windows
	set(OPENSSL_USE_STATIC_LIBS true)
endif()

find_package(ZLIB REQUIRED)
include_directories (${ZLIB_INCLUDE_DIRS})
target_link_libraries(SuperBLT ${ZLIB_LIBRARIES})

find_package (CURL REQUIRED)
include_directories (${CURL_INCLUDE_DIRS})
target_link_libraries(SuperBLT ${CURL_LIBRARY})

find_package (OpenSSL REQUIRED)
include_directories (${OPENSSL_INCLUDE_DIR})
target_link_libraries(SuperBLT ${OPENSSL_LIBRARIES})

find_package (OpenAL REQUIRED)
include_directories (${OPENAL_INCLUDE_DIR})
target_link_libraries(SuperBLT ${OPENAL_LIBRARY})

###############################################################################
## loader binary (this generates a seperate target for both Windows loaders) ##
###############################################################################

# Define the macro
macro(Add_Loader library_name)
	add_library(${library_name} SHARED ${ARGN})
	target_link_libraries(${library_name} LINK_PRIVATE SuperBLT)
	if(UNIX)
		# Use modern C++, and show all warnings
		target_compile_options(${library_name} PUBLIC -Wall -Werror)

		# Use a custom link script, to ensure the correct initialization order
		target_link_libraries(${library_name} LINK_PRIVATE "-Wl,-T,${gnu_link_script}") # Use the link script
		set_target_properties(${library_name} PROPERTIES LINK_DEPENDS ${gnu_link_script}) # Relink if the script is edited

		# Print the build script for debugging - comment out the custom link script and you can use this
		# to retrieve the original script.
		#target_link_libraries(${library_name} "-Wl,--verbose")
	endif()
endmacro()

if(UNIX)
	Add_Loader("superblt_loader" ${gnu_loader_file})
elseif(WIN32)
	Add_Loader("IPHLPAPI" ${win32_loader_path}/iphp.cpp ${win32_loader_path}/iphp.def)
	Add_Loader("WSOCK32" ${win32_loader_path}/wsock.cpp ${win32_loader_path}/wsock.def)
	target_compile_options(IPHLPAPI PRIVATE -DBLT_USE_IPHLPAPI)
	target_compile_options(WSOCK32 PRIVATE -DBLT_USE_WSOCK)
else()
	message(FATAL_ERROR "Unspported OS; if unix based, please add it in CMakeLists.txt")
endif()
