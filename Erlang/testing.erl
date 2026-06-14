-module(testing).
-export([init/0, client/0, jobsRunning/1, routerInit/0, tcpRouter/1]).
-export([request/2, status/1]).
-export([createJob/1, simulateJobs/1]).
-export([parseNodesStr/1, toRecord/1, countRes/1, createReqData/2, getResources/2]).
-include("records.hrl").

init() ->
    spawn(?MODULE, client, []),
    JobsRunning = spawn(?MODULE, jobsRunning, [10]),
    register(jobs_running, JobsRunning),

    Router = spawn(?MODULE, routerInit, []),
    register(tcp_router, Router),

    ok.

% --------------------------------------         

client() ->
    Localhost = "127.0.0.1",
    {ok, Sock} = gen_tcp:connect(Localhost, 5678, [{reuseaddr, true}, {packet, line}, {active, false}]),
    gen_tcp:controlling_process(Sock, tcp_router),
    tcp_router ! {start, Sock},
    tcp_router ! {get_nodes, self()},
    receive
        {nodes, Nodes} -> simulateJobs(Nodes)
    end.

routerInit() ->
    receive
        {start, Sock} -> tcpRouter(Sock)
    end.

tcpRouter(Sock) -> 
    receive
        {get_nodes, ClientPid} ->
            gen_tcp:send(Sock, "GET NODES\n"),
            case gen_tcp:recv(Sock, 0) of
                {ok, "NODES " ++ Rest} -> ClientPid ! {nodes, Rest};
                {ok, _}                -> erlang:error({match_error, "Protocol violation in GET_NODES.~n"});
                {error, Reason}        -> erlang:error({socket_error, Reason})
            end,
            tcpRouter(Sock);

        {req, String, JobId, JobPid} -> 
            gen_tcp:send(Sock, String),
            GrantedPatt = "JOB_GRANTED " ++ JobId ++ "\n",
            DeniedPatt  = "JOB_DENIED " ++ JobId ++ "\n",
            TimeoutPatt = "JOB_TIMEOUT " ++ JobId ++ "\n",
            case gen_tcp:recv(Sock, 0) of
                {ok, GrantedPatt} -> JobPid ! granted;
                {ok, DeniedPatt}  -> JobPid ! denied;
                {ok, TimeoutPatt} -> JobPid ! timeout;
                {ok, _}           -> erlang:error({match_error, "Protocol violation in JOB_REQUEST.~n"});
                {error, Reason}   -> JobPid ! {error, Reason}
            end,
            tcpRouter(Sock);

        {release, String, JobPid} ->
            gen_tcp:send(Sock, String), 
            case gen_tcp:recv(Sock, 0) of
                {ok, Message}   -> JobPid ! {released, Message};
                {error, Reason} -> JobPid ! {error, Reason}
            end,
            tcpRouter(Sock);

        {status, String, JobPid} -> 
            gen_tcp:send(Sock, String),
            case gen_tcp:recv(Sock, 0) of 
                {ok, Message}   -> JobPid ! Message;
                {error, Reason} -> JobPid ! {error, Reason}
            end,
            tcpRouter(Sock);
            
        allDone -> 
            gen_tcp:close(Sock)
    end.

jobsRunning(0)      -> tcp_router ! allDone;
jobsRunning(JobsAm) ->
    receive
        down -> jobsRunning(JobsAm - 1)
    end.

simulateJobs(Nodes) ->
    lists:foreach(fun(_) -> spawn(?MODULE, createJob, [Nodes]), timer:sleep(rand:uniform(5000))  end, lists:seq(1, 10)).

createJob(Nodes) ->
    JobId = integer_to_list(erlang:unique_integer([positive])),
    MaxRes = countRes(Nodes),
    Res = createReqData(Nodes, MaxRes),
    String = "JOB_REQUEST " ++ JobId ++ " " ++ Res,
    request(String, JobId).

request(String, JobId) ->
    tcp_router ! {req, String, JobId, self()},
    receive 
        granted -> 
            timer:sleep(rand:uniform(20000) + 5000),
            tcp_router ! {release, "JOB_RELEASE " ++ JobId ++ "\n", self()},
            receive
                {released, Message} -> 
                    io:fwrite("Resources released: ~p.~n", [Message]),
                    jobs_running ! down;
                {error, Reason} ->
                    jobs_running ! down,
                    erlang:error({release_error, Reason})
            end;

        denied  ->
            io:fwrite("WARNING: No resources for job ~p.~n", [JobId]),
            jobs_running ! down;

        timeout ->
            io:fwrite("WARNING: TIMEOUT for job ~p.~n", [JobId]),
            toDo;

        {error, Reason} -> 
            jobs_running ! down,
            erlang:error({request_error, Reason})
    end,
    ok.

status(String) -> 
    tcp_router ! {status, String, self()},
    receive
        {error, Reason} -> 
            erlang:error({status_error, Reason});
        Msg -> io:fwrite("~p~n", [Msg])
    end,
    ok.

parseNodesStr(NodesData) ->
    % NodesData => "Node;Node..." where Node => ip:port:cpu:x:mem:y:gpu:z
    Nodes = string:tokens(NodesData, ";"),
    NodesParsed = lists:map(fun(X) -> string:tokens(X, ":") end, Nodes),
    lists:map(fun(X) -> toRecord(X) end, NodesParsed). % => [#node{ip, port, cpu, mem, gpu}, ...]

toRecord([Ip, Port, _, CpuVal, _, MemVal, _, GpuVal]) ->
    #node{
        ip = Ip, 
        port = Port, 
        cpu = list_to_integer(CpuVal), 
        mem = list_to_integer(MemVal), 
        gpu = list_to_integer(GpuVal)
    }.

getResources(_, {0, 0, 0}) -> "\n";
getResources([], _) -> erlang:error({parse_error, "ERROR imposible case."});
getResources([#node{
                ip = Ip, 
                port = _, 
                cpu = NodeCpu, 
                mem = NodeMem, 
                gpu = NodeGpu } | NS], 
                {DesCpu, DesMem, DesGpu}) ->
    {StrCpu, NewDesCpu} = if
        (0 < DesCpu) and (DesCpu =< NodeCpu) -> 
            {":cpu:" ++ integer_to_list(DesCpu), 0};
        (0 < DesCpu) and (0 < NodeCpu) -> 
            {":cpu:" ++ integer_to_list(NodeCpu), DesCpu - NodeCpu};
        true ->
            {"", DesCpu}
    end,

    {StrMem, NewDesMem} = if
        (0 < DesMem) and (DesMem =< NodeMem) -> 
            {":mem:" ++ integer_to_list(DesMem), 0};
        (0 < DesMem) and (0 < NodeMem) -> 
            {":mem:" ++ integer_to_list(NodeMem), DesMem - NodeMem};
        true -> 
            {"", DesMem}
    end,

    {StrGpu, NewDesGpu} = if
        (0 < DesGpu) and (DesGpu =< NodeGpu) -> 
            {":gpu:" ++ integer_to_list(DesGpu), 0};
        (0 < DesGpu) and (0 < NodeGpu) -> 
            {":gpu:" ++ integer_to_list(NodeGpu), DesGpu - NodeGpu};
        true -> 
            {"", DesGpu}
    end,

    Res = StrCpu ++ StrMem ++ StrGpu,

    if 
        Res /= "" -> "@" ++ Ip ++ Res ++ " " ++ getResources(NS, {NewDesCpu, NewDesMem, NewDesGpu});
        true -> getResources(NS, {NewDesCpu, NewDesMem, NewDesGpu})
    end.

createReqData(Nodes, {MaxCpu, MaxMem, MaxGpu}) ->
    Rcpu = rand:uniform(MaxCpu + 1) - 1,
    Rgpu = rand:uniform(MaxGpu + 1) - 1,
    Rmem = if
        MaxMem == 0 -> 0;
        true ->
            MaxPow2 = trunc(math:log2(MaxMem)),
            trunc(math:pow(2, rand:uniform(MaxPow2 + 1) - 1))
    end,
    getResources(Nodes, {Rcpu, Rgpu, Rmem}).

countRes([]) -> {0, 0, 0};
countRes([#node{ip = _, port = _, cpu = Cpu, mem = Mem, gpu = Gpu} | NS]) -> 
    {NxtCpu, NxtMem, NxtGpu} = countRes(NS),
    {Cpu + NxtCpu, Mem + NxtMem, Gpu + NxtGpu}.

% TO DO: 
% Validar formato de los nodos enviados por el agente.
% Logs de todo.
% Ver lógica del timeout.
% Separar lógica en archivos y documentar mejor las funciones.