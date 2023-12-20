set(HEADERS
    tinypb_codec.h
    tinypb_data.h
    tinypb_rpc_channel.h
    tinypb_rpc_closure.h
    tinypb_rpc_controller.h
    tinypb_rpc_dispatcher.h
)

install(FILES ${HEADERS} DESTINATION include/myTinyRpc/net/tinypb)