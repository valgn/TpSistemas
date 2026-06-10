-module(testing).
-export([conectarAgenteC/0]).

conectarAgenteC() ->
    Localhost = "127.0.0.1",
    {ok, Sock} = gen_tcp:connect(Localhost, 5678,
                                 [binary, {packet, 0}]),
    ok = gen_tcp:send(Sock, "Echo!"),
    ok = gen_tcp:close(Sock).