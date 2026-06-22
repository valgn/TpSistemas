%%% ----------------------------------------------------------------------
%%% This module contains the implementation of the client and the global processes. The client process is responsible for connecting to its
%%% TCP server and sending request to it.
%%% The global processes are the router, the logger, the resource handler and the job counter. The router is responsible for routing the messages 
%%% between the client and the server; the logger will log the messages sent and received by the client; the resource handler will keep track of 
%%% the avaiable resources that the request jobs can take and the job counter will keep track of the number of jobs that are still running.
%% ----------------------------------------------------------------------
-module(client).
-export([init/0, client/0,tcpRouter/2]).
-export([jobsRunningInit/2, jobsRunning/1, routerInit/1, loggerInit/1, logger/1, resHandlerInit/1, resHandler/1]).
-export([log/1, closeLogger/0, consoleLog/2]).

-include("globals.hrl").
%% ----------------------------------------------------------------------
%% Initializes the global processes (their initializers) and waits for each of them to finally, start the client process.
%% The purpose of this function is to ensure that all the global processes are ready before the client starts sending requests to the server.
%% Parameters: none
%% Returns: This function returns the atom 'init_ok'.
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

%% ----------------------------------------------------------------------
%% Registers and initializes the TCP router process. It also sends a message to the init process to indicate that it has been registered.
%% Parameters: 
%% InitPid Pid of the init process.
%% Return: This function does not return any value.
routerInit(InitPid) ->
    register(tcp_router, self()),
    InitPid ! router_ready,
    receive
        {start, Sock} ->
            inet:setopts(Sock, [{active, true}]),
            tcpRouter(Sock, maps:new())
    end.

%% ----------------------------------------------------------------------
%% Registers and initializes the resource handler process. It also sends a message to the init process to indicate that it has been registered.
%% Parameters: 
%% InitPid -> Pid of the init process.
%% Return: This function does not return any value.
resHandlerInit(InitPid) ->
    register(res_handler, self()),
    InitPid ! res_handler_ready,
    resHandler({0, 0, 0}).

%% ----------------------------------------------------------------------
%% Registers and initializes the job counter process. It also sends a message to the init process to indicate that it has been registered.
%% Parameters: 
%% JobsAm Amount of jobs that are going to be simulated.
%% InitPid Pid of the init process.
%% Return: This function does not return any value.
jobsRunningInit(JobsAm, InitPid) ->
    register(jobs_running, self()),
    InitPid ! job_counter_ready,
    jobsRunning(JobsAm).

%% ----------------------------------------------------------------------
%% Registers and initializes the logger process. It also sends a message to the init process to indicate that it has been registered and 
%% opens/create the log file.
%% Parameters:
%% InitPid -> Pid of the init process.
%% Return: This function does not return any value.
loggerInit(InitPid) ->
    register(logger_proc, self()),
    {ok, FD} = file:open("logs.txt", [append]),
    InitPid ! logger_ready,
    logger(FD).

%% ----------------------------------------------------------------------
%% This function will manage the amount of resources that are available for the jobs. It will be waiting for the initialization message
%% to set the initial amount of resources. Then, it will wait for messages in order to update the availability of the resources using recursion.
%% Parameters: 
%% {Cpu, Mem, Gpu} -> Tuple with the amount of each resource that is currently available.
%% Return: This function does not return any value.
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

%% ----------------------------------------------------------------------
%% This function will manage the amount of the jobs that are currently running. When it reaches 0, it will send a message to the TCP router 
%% process indicating that all the jobs are done.
%% Parameters:
%% JobsAm -> Amount of jobs that are currently running.
%% Return: This function does not return any value.
jobsRunning(0)      -> tcp_router ! allDone;
jobsRunning(JobsAm) ->
    receive
        down -> jobsRunning(JobsAm - 1)
    end.
%% ----------------------------------------------------------------------
%% This function will manage the logging of the messages sent and received by the client. It will be waiting for messages in order to log them
%% Parameters:
%% FD -> File descriptor of the log file.
%% Return: This function does not return any value.
logger(FD) ->
    receive
        {log_it, Data} ->
            io:fwrite(FD, "~s~n", [Data]),
            logger(FD);
        close -> file:close(FD)
    end.

%% -----------------------------------------------------------------------
%% This function will log the messages sent and received by the client into the log file and also print them in the console 
%% if the second Parameterseter is true.
%% Parameters:
%% Msg -> Message to be logged.
%% InConsole -> Boolean that indicates if the message should be printed in the console or not.
%% Return: This function does not return any value.
consoleLog(Msg, InConsole) -> 
    log(Msg),
    if 
        InConsole -> io:fwrite(lists:flatten([Msg, "\n"]), []);
        true -> ok
    end.
%% ----------------------------------------------------------------------
%% This function will send the message to the logger process in order to log it into the log file.
%% Parameters:
%% Data -> Message to be logged.
%% Return: This function does not return any value.
log(Data) ->
    logger_proc ! {log_it, Data}.

%% ----------------------------------------------------------------------
%% This function will send a message to the logger process in order to close the log file.
%% Parameters: none
%% Return: This function does not return any value. 
closeLogger() ->
    logger_proc ! close.

%% ----------------------------------------------------------------------
%% This function is the main client process. It will connect to the TCP server in localhost and receive the nodes in order to start the 
%% simulation of the jobs. In case the TCP connection is closed before receiving the nodes, it will log an error message.
%% Parameters: none
%% Return: This function does not return any value.
client() ->
    Localhost = "127.0.0.1",
    % Firstly, listening passive to avoid receiving messages before the TCP router is ready
    {ok, Sock} = gen_tcp:connect(Localhost, 5678, [{reuseaddr, true}, {packet, line}, {active, false}]), 
    gen_tcp:controlling_process(Sock, whereis(tcp_router)), 
    tcp_router ! {start, Sock},
    tcp_router ! {get_nodes, self()},
    receive
        {nodes, Nodes} -> jobs:simulateJobs(Nodes, ?NUMBERJOBS);
        tcp_closed -> consoleLog("ERROR: TCP closed before receiving nodes.", false)
    end.

%% ----------------------------------------------------------------------
%% This function enroutes petitions and responses throughout an active TCP connection. It issues warnings in case of protocol violations
%% Parameters: 
%% Sock -> Socket of the TCP connection.
%% WorkingMap -> Map that keeps track of the Pids that are waiting for responses.
%% Return: This function does not return any value.
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