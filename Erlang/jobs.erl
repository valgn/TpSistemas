-module(jobs).
-export([request/2, status/2, createJob/1, simulateJobs/2]).
-include("records.hrl").

request(String, JobId) ->
    tcp_router ! {req, String, JobId, self()},
    receive 
        "JOB_GRANTED" -> 
            timer:sleep(rand:uniform(?PLUSTIME) + ?MINTIME),
            tcp_router ! {release, "JOB_RELEASE " ++ JobId ++ "\n", JobId, self()},
            jobs_running ! down;

        "JOB_DENIED"  ->
            io:fwrite("WARNING: No resources for job ~p.~n", [JobId]),
            jobs_running ! down;

        "JOB_TIMEOUT" ->
            io:fwrite("WARNING: TIMEOUT for job ~p.~n", [JobId]),
            toDo;

        {error, Reason} -> 
            jobs_running ! down,
            erlang:error({request_error, Reason});

        _ -> 
            jobs_running ! down,
            erlang:error({command_mismatch, "ERROR: INCORRECT COMMAND"})
        
    end,
    ok.


status(String, JobId) -> 
    tcp_router ! {status, String, JobId, self()},
    receive
        {error, Reason} -> 
            erlang:error({status_error, Reason});
        Msg -> io:fwrite("~p~n", [Msg])
    end,
    ok.


simulateJobs(StrNodes, N) ->
    lists:foreach(fun(_) -> spawn(?MODULE, createJob, [StrNodes]), timer:sleep(rand:uniform(5000))  end, lists:seq(1, N)).


createJob(StrNodes) ->
    JobId          = integer_to_list(erlang:unique_integer([positive])),
    RecNodes       = resourceHandling:parseNodesStr(StrNodes),
    MaxRes         = resourceHandling:countRes(RecNodes), % 
    ReqRes         = resourceHandling:createReqData(RecNodes, MaxRes),
    String         = "JOB_REQUEST " ++ JobId ++ " " ++ ReqRes,
    request(String, JobId).
