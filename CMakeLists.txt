cmake_minimum_required(VERSION 3.0)
project(SuperBLT)

if(CMAKE_SYSTEM_NAME MATCHES "Linux") # Add other systems, if they get PD2 support
	set(UNIX true)
endif()

# TODO set depending on platform
if(UNIX)
	set(library_name superblt_loader)
elseif(WIN32)
	set(library_name IPHLPAPI)
else()
	# TODO raise error
	message(FATAL_ERROR "Unspported OS; if unix based, please add it in CMakeLists.txt")
endif()


# Enable XAudio
add_definitions(-DENABLE_XAUDIO)

###############################################################################
## file globbing ##############################################################
###############################################################################

# these instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variables
# `sources` and `data`
file(GLOB_RECURSE sources		src/*.cpp src/*.c src/*.h)
file(GLOB_RECURSE win32_sources		platform_src/*.cpp platform_src/*.h)
file(GLOB_RECURSE gnu_sources		platforms/gnu/src/*.cpp platforms/gnu/include/*.hh)
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
if(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
	target_compile_options(wren PRIVATE -std=gnu99 -Wall -fPIC)
else()
	target_compile_options(wren PRIVATE -D_CRT_SECURE_NO_WARNINGS)
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

if(UNIX)
	# As the main file has a constructor, make sure it's linked last so it doesn't run before any static constructors
	set(loader_file ${CMAKE_CURRENT_SOURCE_DIR}/platforms/gnu/src/blt_main.cpp)
	list(REMOVE_ITEM gnu_sources ${loader_file})

	add_library(${library_name} SHARED ${sources} ${gnu_sources} ${loader_file})
	include_directories (platforms/gnu/include)

	# Use modern C++, and show all warnings
	target_compile_options(${library_name} PUBLIC -std=c++1y -Wall)

	# Enable this to see missing references:
	#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -z defs")
elseif(WIN32)
	add_library(${library_name} SHARED ${sources} ${win32_sources})
else()
	# TODO raise error
endif()

# Set up the root include directory for the main file
target_include_directories(${library_name} PUBLIC src)

###############################################################################
## dependencies ###############################################################
###############################################################################

target_link_libraries(${library_name} mxml)
target_link_libraries(${library_name} wren)
target_link_libraries(${library_name} subhook)

if(WIN32)
	set(ZLIB_ROOT $ENV{ZLIB_ROOT} $ENV{ZLIB_ROOT}/Release)
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
target_link_libraries(${library_name} ${ZLIB_LIBRARIES})

find_package (CURL REQUIRED)
include_directories (${CURL_INCLUDE_DIRS})
target_link_libraries(${library_name} ${CURL_LIBRARIES})

find_package (OpenSSL REQUIRED)
include_directories (${OPENSSL_INCLUDE_DIR})
target_link_libraries(${library_name} ${OPENSSL_LIBRARIES})

find_package (OpenAL REQUIRED)
include_directories (${OPENAL_INCLUDE_DIR})
target_link_libraries(${library_name} ${OPENAL_LIBRARY})

