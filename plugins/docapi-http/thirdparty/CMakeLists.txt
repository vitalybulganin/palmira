set(PROJECT_NAME usockets)

add_library(${PROJECT_NAME} STATIC
    ${USOCKETS_DIR}/src/bsd.c
    ${USOCKETS_DIR}/src/context.c
    ${USOCKETS_DIR}/src/loop.c
    ${USOCKETS_DIR}/src/socket.c
    ${USOCKETS_DIR}/src/eventing/epoll_kqueue.c
)
set_target_properties(usockets PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(${PROJECT_NAME} PUBLIC ${USOCKETS_DIR}/src)

if (DOCAPI_ENABLE_SSL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC LIBUS_USE_OPENSSL)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenSSL::SSL OpenSSL::Crypto)
else ()
    target_compile_definitions(${PROJECT_NAME} PUBLIC LIBUS_NO_SSL)
endif ()