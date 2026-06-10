-module(testing).
-export([init/0, client/0, genId/1]).
-export([request/2, release/2, status/2]).
-export([createJob/2, simulateJobs/2]).

-include("records.hrl").

init() ->
    register(genIdPid, spawn(?MODULE, genId, [1000])),
    spawn(?MODULE, client, []).

genId(N) ->
    receive
        {getId, Pid} -> 
            Pid ! N,
            genId(N + 1);
        finish -> 
            ok,
            exit(self(), kill)
    end.
% --------------------------------------         

request(Sock, String) ->
    gen_tcp:send(Sock, String),
    case gen_tcp:recv(Sock, 0) of
        {ok, "JOB_GRANTED " ++ JobId} -> % Simular Trabajo
        {ok, "JOB_DENIED "  ++ JobId} -> % No tenes recursos
        {ok, "JOB_TIMEOUT " ++ JobId} -> % TIMEOUT
        {error, Reason}               -> % Error
    end.

release(Sock, String) ->
    gen_tcp:send(Sock, String),
    case gen_tcp:recv(Sock, 0) of
        {ok, Message}   -> Message;
        {error, Reason} -> %Reason
    end.


status(Sock, String) -> 


client() ->
    Localhost = "127.0.0.1",
    {ok, Sock} = gen_tcp:connect(Localhost, 5678, [binary, {packet, 0}]),
    ok = gen_tcp:send(Sock, "GET_NODES"),
    case gen_tcp:recv(Sock, 0) of
        {ok, String}    -> %parsear
        {error, closed} -> %handelear
    
    simulateJobs(Sock),
    
    gen_tcp:close(Sock),
    genIdPid ! finish,
    ok.

    
createJob(Sock, Nodes) ->
    genIdPid ! {genId, self()}, % Pedimos un Id a genPid
    receive
        N ->    
            String = "JOB_REQUEST" ++ integer_to_list(N) ++ %Resta,
            request(Sock, String),
            release(Sock, "JOB_RELEASE" ++ integer_to_list(N))
    end.


simulateJobs(Sock, Nodes) ->
    lists:foreach(spawn(?MODULE, createJob, [Sock, Nodes]), lists:seq(1, 5)).

% TO DO: 
% Parsear string que es enviado por Agente C en forma de records [#nodo {ip, cpu, mem, gpu}]
% Simular (con la lista anterior) trabajos en createJob, crear una funcion auxiliar que agarre de forma random recursos de nodos
% Manejar Errores, ver que onda con status, manejar respuestas que llegan a request()