set(HEADERS
    abstract_slot.h
    io_thread.h
    tcp_buffer.h
    tcp_client.h
    tcp_connection_time_wheel.h
    tcp_connection.h
    tcp_server.h
)

install(FILES ${HEADERS} DESTINATION include/myTinyRpc/net/tcp)