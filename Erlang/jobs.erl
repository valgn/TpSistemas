%%% -----------------------------------------------------------------------
%%% This module contains the implementation of the job processes, the simulation of them and the request/release petitions.
%%% -----------------------------------------------------------------------
-module(jobs).
-export([request/3, status/2, createJob/1, simulateJobs/2, handleResInit/1, getRandAvRes/0, releaseRes/1, shuffle/1]).
-include("globals.hrl").

%% ------------------------------------------------------------------------
%% Sends a request message to the TCP router with the request string, which contains the resources desired for the job.
%% Waits for a response from the TCP router and handles it accordingly. Everytime a request ends, it sends a message to the jobs_running process 
%% to decrement the number of running jobs. 
%% Parameters:
%% String The request string to be sent to the TCP router.
%% JobId The unique identifier of the job.
%% DesRes The desired resources for the job, used to release them in case of a timeout or a denied request.
%% Return : This function does not return any value.
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

%% ------------------------------------------------------------------------
%% Sends a status message to the TCP router with the status string.
%% Parameters:
%% String The status string to be sent to the TCP router.
%% JobId The unique identifier of the job.
%% Return : This function does not return any value.
status(String, JobId) -> 
    tcp_router ! {status, String, JobId, self()},
    receive
        {error, Reason} -> erlang:error({status_error, Reason});
        _ -> ok
    end.

%% ------------------------------------------------------------------------
%% Initializes the resource handler process with the maximum resources available from the nodes.
%% Parameters:
%% MaxRes A tuple containing the maximum resources available from the nodes, in the format {Cpu, Mem, Gpu}.
%% Return : This function does not return any value.    
handleResInit(MaxRes) -> res_handler ! {init, MaxRes}.

%% ------------------------------------------------------------------------
%% Sends a release message to the resource handler process with the resources to be released.
%% Parameters: 
%% Res A tuple containing the resources to be released, in the format {Cpu, Mem, Gpu}.
%% Return : This function does not return any value.
releaseRes(Res) -> res_handler ! {rel, Res}.

%% ------------------------------------------------------------------------
%% Sends a message to the resource handler process to get the currently available resources. Waits for the response and returns it as a tuple 
%% in the format {Cpu, Mem, Gpu}.
%% Parameters: none
%% Return: A tuple containing the currently available resources.
getRandAvRes() -> 
    res_handler ! {get, self()},
    receive
        {Cpu, Mem, Gpu} -> {Cpu, Mem, Gpu}
    end.

%% ------------------------------------------------------------------------
%% Shuffles the list of nodes randomly. This is used to avoid requesting multiple times the same resources to the same nodes.
%% Parameters:
%% Nodes A list of nodes to be shuffled. Each node is a record in the format #node{ip, port, cpu, mem, gpu}.
%% Return: A list of nodes shuffled randomly.   
shuffle(Nodes) ->
    PairedNodes = [{rand:uniform(), N} || N <- Nodes],
    SortedNodes = lists:sort(PairedNodes),
    [N || {_, N} <- SortedNodes].

%% ------------------------------------------------------------------------
%% Simulates the creation of jobs by parsing the nodes from a string to a list of records in the format #node{ip, port, cpu, mem, gpu}. 
%% Initializes the resource handler process with the maximum resources available. Then, it creates N jobs.
%% Parameters:
%% StrNodes A string containing the nodes information, in the format "ip:port:cpu:mem:gpu,ip:port:cpu:mem:gpu,...".
%% N The number of jobs to be created in the simulation.
%% Return: This function does not return any value.
simulateJobs(StrNodes, N) ->
    RecNodes = resourceHandling:parseNodesStr(StrNodes),
    MaxRes = resourceHandling:countRes(RecNodes),
    handleResInit(MaxRes),
    lists:foreach(fun(_) -> spawn(?MODULE, createJob, [RecNodes]), timer:sleep(rand:uniform(5000))  end, lists:seq(1, N)),
    ok.

%% ------------------------------------------------------------------------
%% Creates a job by generating an unique JobId, getting the currently available resources. Shuffles the nodes to prevent collisions and 
%% creates the final string to generate the request.
%% Parameters:
%% RecNodes A list of records containing the nodes information.
%% Return: This function does no return any value. 
createJob(RecNodes) ->
    JobId = integer_to_list(erlang:unique_integer([positive])),
    DesRes = getRandAvRes(),
    ShuffledNodes = shuffle(RecNodes),
    ReqRes = resourceHandling:getResourcesStr(ShuffledNodes, DesRes),
    String = lists:flatten(["JOB_REQUEST ", JobId, " ", ReqRes]),
    request(String, JobId, DesRes).
