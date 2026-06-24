cmake_minimum_required(VERSION 3.5)
project(palmira)

find_package(MoonyCode REQUIRED)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

# Set output folders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE PATH "Single Directory for all")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE PATH "Single Directory for all")

if(MSVC)
	set(XXX_SUF -win)
else()
	set(XXX_SUF -nix)
endif()

# Options
option(SANITIZE_ENABLED "Enable sanitize" OFF)
option(PROFILER_ENABLED "Enable profile" OFF)
option(gRPC_API_ENABLED "Enable gRPC support" OFF)
option(HTTP_API_ENABLED "Enable HTTP/1.1 support" ON)
option(BUILD_THIRDPARTY_MODULES "Build third-party modules" ON)

# Checking dependency modules.
#-------------------------------------------------------------------------#
include(cmake/dependencies.cmake)
#-------------------------------------------------------------------------#

# add_compile_options(-DDEBUG_TOOLS)
#
add_compile_options(-mfpmath=sse -fstack-protector-all -g
	-W -Wall -Wextra -Wcast-align -pedantic -Wfloat-equal -Wpointer-arith -Wformat-security
	-Wmissing-format-attribute -Wformat=1 -Wwrite-strings -Wcast-align -Wno-long-long
	-Woverloaded-virtual -Wno-suggest-attribute=format)

if (SANITIZE_ENABLED)
	add_compile_options(-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak)
	add_link_options(-fsanitize=address)
endif()

if (PROFILER_ENABLED)
	add_compile_options(-p -pg)
	add_link_options(-p -pg)
endif()

include_directories(
	./
	contrib)

set(GRPC_LIBS)
set(HTTP_LIBS)

if (BUILD_THIRDPARTY_MODULES)
    add_subdirectory(contrib)
endif (BUILD_THIRDPARTY_MODULES)

add_subdirectory(plugins/morpho-ru)

if(gRPC_API_ENABLED)
	include(src/network/grpc/cmake/grpc.cmake)

	set(PROTO ${PROJECT_SOURCE_DIR}/src/network/grpc/proto/grpcttp.proto)

	set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.pb.cc")
	set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.pb.h")
	set(PROTO_GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.grpc.pb.cc")
	set(PROTO_GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.grpc.pb.h")

	set(PROTOBUF_PROTOC protoc)

	set(GRPC_SRC
		src/network/grpc/messages.cpp
		src/network/grpc/server.cpp
		src/network/grpc/client.cpp)

	add_custom_command(
		OUTPUT "${PROTO_SRC}" "${PROTO_HDR}" "${PROTO_GRPC_SRC}" "${PROTO_GRPC_HDR}"
		COMMAND ${PROTOBUF_PROTOC}
		ARGS
			--grpc_out   "${CMAKE_CURRENT_BINARY_DIR}"
			--cpp_out    "${CMAKE_CURRENT_BINARY_DIR}"
			--proto_path "${PROJECT_SOURCE_DIR}/src/network/grpc/proto"
			--plugin=protoc-gen-grpc=`which grpc_cpp_plugin`
			"${PROTO}"
		DEPENDS "${PROTO}")

	include_directories("${CMAKE_CURRENT_BINARY_DIR}")

	add_library(grpcapi
		${GRPC_SRC}
		${PROTO_SRC}
		${PROTO_HDR}
		${PROTO_GRPC_SRC}
		${PROTO_GRPC_HDR})

	target_link_libraries(grpcapi
		absl::check
		${_REFLECTION}
		${_GRPC_GRPCPP}
		${_PROTOBUF_LIBPROTOBUF})

	set(GRPC_LIBS grpcapi)

	add_compile_options(-DgRPC_API_ENABLED)
endif()

if(HTTP_API_ENABLED)
    set(HTTP_LIBS httpapi remottp)

    add_subdirectory(
		contrib/remottp)
	add_library(httpapi
		src/network/http/jsload.cpp
		src/network/http/zmload.cpp
		src/network/http/unpack.cpp
		src/network/http/server.cpp
		src/network/http/client.cpp)
endif()

add_library(tripoli
	src/objects/read-zip.cpp
	src/objects/doc-zmap.cpp

	src/service/structo-create.cpp
	src/service/structo-search.cpp
	src/service/collect-docs.cpp
	src/service/collect-quotes.cpp

	src/toolset/plugins.cpp
	src/toolset/toolset.cpp
	src/toolset/z-arguments.cpp)

add_library(watchFs
	src/watchFs/watchFs${XXX_SUF}.cpp)

add_executable(palmira
	src/palmira.cpp
	src/netServer.cpp)

#[[<TODO>
add_executable(k2
	src/k2.cpp)
target_link_libraries(k2
	remottp
	tripoli
	structo
	DeliriX
	watchFs
	${MoonyCode_LIB}
	mtc
	tinyxml2
	minizip
	z
	pthread
	dl)
]]

target_link_libraries(palmira
	${GRPC_LIBS}
	${HTTP_LIBS}
	tripoli
	structo
	DeliriX
	${MoonyCode_LIB}
	mtc
	tinyxml2
	minizip
	z
	pthread
	dl)

#<TODO> add_subdirectory(utils)
#<TODO> add_subdirectory(tests)
add_subdirectory(plugins)
