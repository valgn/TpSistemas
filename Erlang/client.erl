-module(client).
-export([init/0, client/0,tcpRouter/2]).
-export([jobsRunningInit/2, jobsRunning/1, routerInit/1, loggerInit/1, logger/1, resHandlerInit/1, resHandler/1]).
-export([log/1, closeLogger/0, consoleLog/2]).
-include("globals.hrl").


init() ->
    spawn(?MODULE, routerInit, [self()]),
    receive 
        router_ready -> ok
    end,

    spawn(?MODULE, resHandlerInit, [self()]),
    receive 
        res_handler_ready -> ok
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

    init_ok.


resHandlerInit(InitPid) ->
    register(res_handler, self()),
    InitPid ! res_handler_ready,
    resHandler({0, 0, 0}).


resHandler({Cpu, Mem, Gpu}) ->
    receive
        {init, Res} -> resHandler(Res);
        {get, Pid} -> 
            {DesC, DesM, DesG} = resourceHandling:createReqData({Cpu, Mem, Gpu}),
            Pid ! {DesC, DesM, DesG},
            resHandler({Cpu - DesC, Mem - DesM, Gpu - DesG});
        {rel, {C, M, G}} -> resHandler({Cpu + C, Mem + M, Gpu + G});
        finish -> ok
    end.


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
            io:fwrite(FD, "~s~n", [Data]),
            logger(FD);
        close -> file:close(FD)
    end.


consoleLog(Msg, InConsole) -> 
    log(Msg),
    if 
        InConsole -> io:fwrite(lists:flatten([Msg, "\n"]), []);
        true -> ok
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
        {nodes, Nodes} -> jobs:simulateJobs(Nodes, ?NUMBERJOBS);
        tcp_closed -> consoleLog("ERROR: TCP closed before receiving nodes.", false)
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
            consoleLog("GET_NODES", true),
            NewMap = maps:put(req_nodes, ClientPid, WorkingMap),
            tcpRouter(Sock, NewMap);

        {req, RequestData, JobId, JobPid} ->
            gen_tcp:send(Sock, RequestData ++ "\n"),
            consoleLog(RequestData, true),
            NewMap = maps:put(JobId, JobPid, WorkingMap),
            tcpRouter(Sock, NewMap);

        {release, ReleaseData, _JobId, _JobPid} ->
            gen_tcp:send(Sock, ReleaseData ++ "\n"),
            consoleLog(ReleaseData, true),
            tcpRouter(Sock, WorkingMap); 

        {status, StatusData ,JobId, JobPid} ->
            gen_tcp:send(Sock, StatusData ++ "\n"),
            consoleLog(StatusData, true),
            NewMap = maps:put(JobId, JobPid, WorkingMap),
            tcpRouter(Sock, NewMap);

        {tcp, Sock, Data} ->
            CleanData = string:trim(Data),
            consoleLog(CleanData, true),
            case string:split(CleanData, " ") of
                ["NODES", NodesData] -> 
                    case maps:find(req_nodes, WorkingMap) of
                        {ok, ClientPid} -> 
                            ClientPid ! {nodes, NodesData},
                            tcpRouter(Sock, maps:remove(req_nodes, WorkingMap));
                        error -> 
                            consoleLog("WARNING: JobId not found in TCP router.", false),
                            tcpRouter(Sock, WorkingMap)
                    end;
                [Comm, JobId] ->
                    case maps:find(JobId, WorkingMap) of
                        {ok, JobPid} -> 
                            JobPid ! Comm,
                            tcpRouter(Sock, maps:remove(JobId, WorkingMap));
                        error -> 
                            consoleLog("WARNING: JobId not found in TCP router.", false),
                            tcpRouter(Sock, WorkingMap)
                    end;
                _ -> 
                    consoleLog("WARNING: Bad match in TCP router.", false),
                    tcpRouter(Sock, WorkingMap)
            end;

        {tcp_closed, Sock} ->
            consoleLog("TCP closed.", true),
            closeLogger(),
            res_handler ! finish,
            lists:foreach(fun ({_, Pid}) -> Pid ! tcp_closed end, maps:to_list(WorkingMap)),
            ok;

        allDone -> 
            closeLogger(),
            gen_tcp:close(Sock)
    end.

% TO DO: 
% Testear.
% Documentar mejor las funciones.