set(HEADERS
    abstract_codec.h
    abstract_data.h 
    abstract_dispatcher.h
    mutex.h
    reactor.h
    fd_event.h
    net_address.h
    timer.h
)

install(FILES ${HEADERS} DESTINATION include/myTinyRpc/net)