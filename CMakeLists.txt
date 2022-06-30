cmake_minimum_required(VERSION 3.2)

project(baikal-scp C)
include(GNUInstallDirs)

# Driver version
set(BAIKAL_SCP_DRV_VERSION_MAJOR 1)
set(BAIKAL_SCP_DRV_VERSION_MINOR 0)
set(BAIKAL_SCP_DRV_VERSION_PATCH 0)

# Shared librrary version
set(BAIKAL_SCP_LIB_VERSION_MAJOR 1)
set(BAIKAL_SCP_LIB_VERSION_MINOR 0)
set(BAIKAL_SCP_LIB_VERSION_PATCH 0)
set(BAIKAL_SCP_LIB_VERSION_STRING
	${BAIKAL_SCP_LIB_VERSION_MAJOR}.${BAIKAL_SCP_LIB_VERSION_MINOR}.${BAIKAL_SCP_LIB_VERSION_PATCH})

# CLI utility version
set(BAIKAL_SCP_TOOL_VERSION_MAJOR 1)
set(BAIKAL_SCP_TOOL_VERSION_MINOR 1)
set(BAIKAL_SCP_TOOL_VERSION_PATCH 0)

option(USE_LIBCURL "Build with libcurl support" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

add_definitions(-O2 -Wall -Werror --std=gnu99 -D_GNU_SOURCE)
include_directories(
	include
	userspace/include
	userspace/lib
	userspace/tool
)

# Look for required libraries
SET(requiredlibs)

if(USE_LIBCURL)
	find_package(CURL)
	if(CURL_FOUND)
		include_directories(${CURL_INCLUDE_DIR})
		set(requiredlibs ${requiredlibs} ${CURL_LIBRARIES} )
	else(CURL_FOUND)
		message(FATAL_ERROR "Could not find the CURL library and development files.")
	endif(CURL_FOUND)
endif(USE_LIBCURL)

if(USE_LIBCURL)
	add_definitions(-DUSE_LIBCURL)
endif(USE_LIBCURL)

# Kernel module
find_package(KernelHeaders REQUIRED)

set(
	KBUILD_CMD
		${CMAKE_MAKE_PROGRAM} -C ${KERNELHEADERS_DIR}
		M=${CMAKE_CURRENT_BINARY_DIR}
		src=${CMAKE_CURRENT_SOURCE_DIR}/kernel
)

add_custom_target(
	baikal-scp
	COMMAND ${CMAKE_COMMAND} -E env "KCFLAGS=-DBAIKAL_SCP_DRV_VERSION_MAJOR=${BAIKAL_SCP_DRV_VERSION_MAJOR} -DBAIKAL_SCP_DRV_VERSION_MINOR=${BAIKAL_SCP_DRV_VERSION_MINOR} -DBAIKAL_SCP_DRV_VERSION_PATCH=${BAIKAL_SCP_DRV_VERSION_PATCH}"
	${KBUILD_CMD} modules
)

add_custom_target(
	baikal-scp-clean
	COMMAND ${KBUILD_CMD} clean
)

# Shared library
add_library(baikal-scp-lib SHARED
	userspace/lib/baikal_scp_lib.c
	userspace/lib/baikal_scp_lib_flash.c
)

target_compile_definitions(baikal-scp-lib PUBLIC
	-DBAIKAL_SCP_LIB_VERSION_MAJOR=${BAIKAL_SCP_LIB_VERSION_MAJOR}
	-DBAIKAL_SCP_LIB_VERSION_MINOR=${BAIKAL_SCP_LIB_VERSION_MINOR}
	-DBAIKAL_SCP_LIB_VERSION_PATCH=${BAIKAL_SCP_LIB_VERSION_PATCH}
)

set_target_properties(baikal-scp-lib PROPERTIES
	VERSION   ${BAIKAL_SCP_LIB_VERSION_STRING}
	SOVERSION ${BAIKAL_SCP_LIB_VERSION_STRING}
)

# CLI utility
add_executable(baikal-scp-flash
	userspace/tool/baikal_scp_flash.c
)

target_compile_definitions(baikal-scp-flash PUBLIC
	-DBAIKAL_SCP_TOOL_VERSION_MAJOR=${BAIKAL_SCP_TOOL_VERSION_MAJOR}
	-DBAIKAL_SCP_TOOL_VERSION_MINOR=${BAIKAL_SCP_TOOL_VERSION_MINOR}
	-DBAIKAL_SCP_TOOL_VERSION_PATCH=${BAIKAL_SCP_TOOL_VERSION_PATCH}
)

target_link_libraries(baikal-scp-flash baikal-scp-lib ${requiredlibs})

install(TARGETS baikal-scp-flash RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
install(TARGETS baikal-scp-lib LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/baikal_scp_lib.h DESTINATION /usr/include)
