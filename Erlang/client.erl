-module(client).
-export([init/0, client/0, jobsRunningInit/2, jobsRunning/1, routerInit/1, tcpRouter/2, loggerInit/1, logger/1]).
-export([log/1, closeLogger/0]).
-include("records.hrl").


init() ->
    spawn(?MODULE, routerInit, [self()]),
    receive 
        router_ready -> ok
    end,

    spawn(?MODULE, loggerInit, [self()]),
    receive 
        logger_ready -> ok
    end,

    spawn(?MODULE, jobsRunningInit, [?NUMBERJOBS, self()]),
    receive
        job_counter_ready -> ok
    end,
    
    spawn(?MODULE, client, []),

    ok.

jobsRunningInit(JobsAm, InitPid) ->
    register(jobs_running, self()),
    InitPid ! job_counter_ready,
    jobsRunning(JobsAm).

jobsRunning(0)      -> tcp_router ! allDone;
jobsRunning(JobsAm) ->
    receive
        down -> jobsRunning(JobsAm - 1)
    end.

loggerInit(InitPid) ->
    register(logger_proc, self()),
    {ok, FD} = file:open("logs.txt", [append]),
    InitPid ! logger_ready,
    logger(FD).

logger(FD) ->
    receive
        {loggear, Data} ->
            io:fwrite(FD, "~s", [Data]),
            logger(FD);
        close -> file:close(FD)
    end.

log(Data) ->
    logger_proc ! {loggear, Data}.
closeLogger() ->
    logger_proc ! close.

client() ->
    Localhost = "127.0.0.1",
    {ok, Sock} = gen_tcp:connect(Localhost, 5678, [{reuseaddr, true}, {packet, line}, {active, false}]),
    gen_tcp:controlling_process(Sock, whereis(tcp_router)),
    tcp_router ! {start, Sock},
    tcp_router ! {get_nodes, self()},
    receive
        {nodes, Nodes} -> jobs:simulateJobs(Nodes, ?NUMBERJOBS)
    end.


routerInit(InitPid) ->
    register(tcp_router, self()),
    InitPid ! router_ready,
    receive
        {start, Sock} ->
            inet:setopts(Sock, [{active, true}]),
            tcpRouter(Sock, maps:new())
    end.


tcpRouter(Sock, WorkingMap) -> 
    receive 
        {get_nodes, ClientPid} ->
            gen_tcp:send(Sock, "GET_NODES\n"),
            log("GET_NODES\n"),
            NewMap = maps:put(req_nodes, ClientPid, WorkingMap),
            tcpRouter(Sock, NewMap);

        {req, RequestData, JobId, JobPid} ->
            gen_tcp:send(Sock, RequestData),
            log(RequestData),
            NewMap = maps:put(JobId, JobPid, WorkingMap),
            tcpRouter(Sock, NewMap);

        {release, ReleaseData, _JobId, _JobPid} ->
            gen_tcp:send(Sock, ReleaseData),
            log(ReleaseData),
            tcpRouter(Sock, WorkingMap); 

        {status, StatusData ,JobId, JobPid} ->
            gen_tcp:send(Sock, StatusData),
            log(StatusData),
            NewMap = maps:put(JobId, JobPid, WorkingMap),
            tcpRouter(Sock, NewMap);

        {tcp, Sock, Data} ->
            CleanData = string:trim(Data),
            log([CleanData, "\n"]),
            case string:tokens(CleanData, " ") of
                ["NODES", NodesData] -> 
                    case maps:find(req_nodes, WorkingMap) of
                        {ok, ClientPid} -> 
                            ClientPid ! {nodes, NodesData},
                            tcpRouter(Sock, maps:remove(req_nodes, WorkingMap));
                        error -> 
                            log("WARNING: JOBID NOT FOUND IN TCP ROUTER\n"),
                            tcpRouter(Sock, WorkingMap)
                    end;
                [Comm, JobId] ->
                    case maps:find(JobId, WorkingMap) of
                        {ok, JobPid} -> 
                            JobPid ! Comm,
                            tcpRouter(Sock, maps:remove(JobId, WorkingMap));
                        error -> 
                            log("WARNING: JOBID NOT FOUND IN TCP ROUTER\n"),
                            tcpRouter(Sock, WorkingMap)
                    end;
                _ -> 
                    log("WARNING: BADMATCH IN TCP ROUTER\n"),
                    tcpRouter(Sock, WorkingMap)
            end;

        {tcp_closed, Sock} ->
            log("TCP CLOSED\n"),
            closeLogger(),
            ok;

        allDone -> 
            closeLogger(),
            gen_tcp:close(Sock)
    end.

% TO DO: 
% Ver lógica del timeout.
% Documentar mejor las funciones.
% Ver si es necesario que el cliente vuelva a ejecutarse.