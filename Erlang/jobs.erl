-module(jobs).
-export([request/3, status/2, createJob/1, simulateJobs/2, handleResInit/1, getRandAvRes/0, releaseRes/1, shuffle/1]).
-include("globals.hrl").


request(String, JobId, DesRes) ->
    tcp_router ! {req, String, JobId, self()},
    receive 
        "JOB_GRANTED" -> 
            timer:sleep(rand:uniform(?JOB_PLUSTIME) + ?JOB_MINTIME),
            tcp_router ! {release, "JOB_RELEASE " ++ JobId, JobId, self()},
            jobs_running ! down,
            releaseRes(DesRes);

        "JOB_DENIED"  ->
            jobs_running ! down,
            releaseRes(DesRes);

        "JOB_TIMEOUT" ->
            releaseRes(DesRes),
            timer:sleep(rand:uniform(?PLUS_TIMEOUT) + ?MIN_TIMEOUT),
            request(String, JobId, DesRes);
        
        tcp_closed -> 
            jobs_running ! down,
            client:consoleLog(lists:flatten(["ERROR: TCP closed in job ", JobId, "."]), true);

        _ -> 
            jobs_running ! down,
            client:consoleLog(lists:flatten(["ERROR: Incorrect command in job ", JobId, "."]), true)
    end.


status(String, JobId) -> 
    tcp_router ! {status, String, JobId, self()},
    receive
        {error, Reason} -> erlang:error({status_error, Reason});
        _ -> ok
    end.


handleResInit(MaxRes) -> res_handler ! {init, MaxRes}.


releaseRes(Res) -> res_handler ! {rel, Res}.


getRandAvRes() -> 
    res_handler ! {get, self()},
    receive
        {Cpu, Mem, Gpu} -> {Cpu, Mem, Gpu}
    end.


shuffle(Nodes) ->
    PairedNodes = [{rand:uniform(), N} || N <- Nodes],
    SortedNodes = lists:sort(PairedNodes),
    [N || {_, N} <- SortedNodes].


simulateJobs(StrNodes, N) ->
    RecNodes = resourceHandling:parseNodesStr(StrNodes),
    MaxRes = resourceHandling:countRes(RecNodes),
    handleResInit(MaxRes),
    lists:foreach(fun(_) -> spawn(?MODULE, createJob, [RecNodes]), timer:sleep(rand:uniform(5000))  end, lists:seq(1, N)).


createJob(RecNodes) ->
    JobId = integer_to_list(erlang:unique_integer([positive])),
    DesRes = getRandAvRes(),
    ShuffledNodes = shuffle(RecNodes),
    ReqRes = resourceHandling:getResourcesStr(ShuffledNodes, DesRes),
    String = lists:flatten(["JOB_REQUEST ", JobId, " ", ReqRes]),
    request(String, JobId, DesRes).
