cmake_minimum_required(VERSION 3.0)
project(SuperBLT)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(UNIX true)
endif()

# TODO set depending on platform
set(library_name IPHLPAPI)

###############################################################################
## file globbing ##############################################################
###############################################################################

# these instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variables
# `sources` and `data`
file(GLOB_RECURSE sources		src/*.cpp src/*.h)
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
target_compile_options(mxml PRIVATE -std=c99 -Wall -fPIC)
target_include_directories(mxml PUBLIC lib/mxml)

file(GLOB_RECURSE wren_sources		lib/wren/src/include/wren.h lib/wren/src/vm/*.c lib/wren/src/vm/*.h lib/wren/src/optional/*.c lib/wren/src/optional/*.h)
add_library(wren STATIC ${wren_sources})
include_directories(wren lib/wren/src/optional lib/wren/src/vm)
target_compile_options(wren PRIVATE -std=c99 -Wall -fPIC)
target_include_directories(wren PUBLIC lib/wren/src/include)

if(UNIX)
	add_library(${library_name} SHARED ${sources} ${gnu_sources})
	target_compile_options(${library_name} PUBLIC -std=c++1y -Wall)
elseif(WIN32)
	add_library(${library_name} SHARED ${sources} ${win32_sources})
else()
	# TODO raise error
endif()

# Set up the root include directory for the main file
target_include_directories(${library_name} PUBLIC src)

# this copies all resource files in the build directory
# we need this, because we want to work with paths relative to the executable
file(COPY ${data} DESTINATION resources)

###############################################################################
## dependencies ###############################################################
###############################################################################

target_link_libraries(${library_name} mxml)
target_link_libraries(${library_name} wren)

if(UNIX)
	find_package (CURL REQUIRED)
	include_directories (${CURL_INCLUDE_DIRS})
	target_link_libraries(${library_name} ${CURL_LIBRARIES})

	find_package (OpenSSL REQUIRED)
	include_directories (${OPENSSL_INCLUDE_DIRS})
	target_link_libraries(${library_name} ${OPENSSL_LIBRARIES})
endif()

target_link_libraries(${library_name} PUBLIC
	${Boost_LIBRARIES}
	# here you can add any library dependencies
)

