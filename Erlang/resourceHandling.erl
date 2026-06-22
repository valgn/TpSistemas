%%% ------------------------------------------------------------------------
%%% This module provides parsing and resource managing utilities.
%%% It translates TCP strings into records.
%%% ------------------------------------------------------------------------
-module(resourceHandling).
-export([parseNodesStr/1, getNodeRes/2, toRecord/1, getResourcesStr/2, createReqData/1, countRes/1]).
-include("globals.hrl").

%% ------------------------------------------------------------------------
%% Parses a string of semicolon-separated nodes into a list of records.
%% Parameters:
%% NodesData A string containing the nodes information, in the format "ip:port:cpu:x:mem:y:gpu:z;ip:port:cpu:x:mem:y:gpu:z  
%% Return: A list of records containing the nodes information.
parseNodesStr(NodesData) ->
    % NodesData => "Node;Node..." where Node => ip:port:cpu:x:mem:y:gpu:z
    Nodes = string:tokens(NodesData, ";"), % string:lexemes
    NodesParsed = lists:map(fun(X) -> string:tokens(X, ":") end, Nodes),
    lists:map(fun(X) -> toRecord(X) end, NodesParsed). % => [#node{ip, port, cpu, mem, gpu}, ...]

%% ------------------------------------------------------------------------
%% Converts a tokenized node string into a record.
%% Fails if the format is invalid.
%% Parameters:
%% A list of strings containing the node information, in the format [ip, port, "cpu", x, "mem", y, "gpu", z].
%% Return: A record containing the node information, in the format #node{ip, port, cpu, mem, gpu}.
toRecord([Ip, Port | Resources]) ->
    AvRes = getNodeRes(Resources, #{cpu => 0, mem => 0, gpu => 0}),
    #node{
        ip  = Ip,
        port  = Port,
        cpu = maps:get(cpu, AvRes),
        mem = maps:get(mem, AvRes),
        gpu = maps:get(gpu, AvRes)
    };
toRecord(_) -> erlang:error({format_error, "ERROR: Invalid format."}).

%% ------------------------------------------------------------------------
%% Extracts the resources from a node and returns them as a map. The input is a list of strings in the format ["cpu", x, "mem", y, "gpu", z].
%% Parameters:
%% A list of strings containing the resources information, in the format ["cpu", x, "mem", y, "gpu", z].
%% Map -> A map to store the resources, initialized with #{cpu => 0, mem => 0, gpu => 0}.
%% Return: A map containing the resources information, in the format #{cpu => x, mem => y, gpu => z}.
getNodeRes([], Map) -> 
    Map;
getNodeRes(["cpu", Val | Rest], Map) ->
    getNodeRes(Rest, maps:put(cpu, list_to_integer(Val), Map));
getNodeRes(["mem", Val | Rest], Map) ->
    getNodeRes(Rest, maps:put(mem, list_to_integer(Val), Map));
getNodeRes(["gpu", Val | Rest], Map) ->
    getNodeRes(Rest, maps:put(gpu, list_to_integer(Val), Map));
getNodeRes(_, _) -> erlang:error({format_error, "ERROR: Invalid format."}).

%% ------------------------------------------------------------------------
%% Creates a random resource amount to be requested by a job, based on the maximum resources available.
%% Parameters:
%% A tuple containing the maximum resources available from the nodes, in the format {Cpu, Mem, Gpu}.
%% Return: A tuple containing the random resources to be requested, in the format {Cpu, Mem, Gpu}.
createReqData({MaxCpu, MaxMem, MaxGpu}) ->
    Rcpu = rand:uniform(MaxCpu + 1) - 1,
    Rgpu = rand:uniform(MaxGpu + 1) - 1,
    Rmem = if
        MaxMem == 0 -> 0;
        true ->
            MaxPow = trunc(math:log2(MaxMem)),
            Min = if MaxPow < 6 -> MaxPow; true -> 6 end,
            trunc(math:pow(2, Min + rand:uniform(MaxPow - Min + 1) - 1))
    end,
    {Rcpu, Rmem, Rgpu}.

%% ------------------------------------------------------------------------
%% Recursively builds a string of resources to be requested based on the desired and available resource of the nodes. Itereates over the list of nodes
%% subtracting the desired capacity until it is satisfied.
%% Parameters: 
%% Nodes A list of records containing the nodes information.
%% DesiredRes A tuple containing the desired resources to be requested
%% Return: A string containing the resources to be requested 
getResourcesStr(_, {0, 0, 0}) -> "";
getResourcesStr([#node{
                ip = Ip, 
                port = _, 
                cpu = NodeCpu, 
                mem = NodeMem, 
                gpu = NodeGpu } | NS], 
                {DesCpu, DesMem, DesGpu}) ->
    {StrCpu, NewDesCpu} = if
        (0 < DesCpu) and (DesCpu =< NodeCpu) -> 
            {lists:flatten(["@", Ip, ":cpu:", integer_to_list(DesCpu)]), 0};
        (0 < DesCpu) and (0 < NodeCpu) -> 
            {lists:flatten(["@", Ip, ":cpu:", integer_to_list(NodeCpu)]), DesCpu - NodeCpu};
        true ->
            {"", DesCpu}
    end,

    {StrMem, NewDesMem} = if
        (0 < DesMem) and (DesMem =< NodeMem) -> 
            {lists:flatten(["@", Ip, ":mem:", integer_to_list(DesMem)]), 0};
        (0 < DesMem) and (0 < NodeMem) -> 
            {lists:flatten(["@", Ip, ":mem:", integer_to_list(NodeMem)]), DesMem - NodeMem};
        true -> 
            {"", DesMem}
    end,

    {StrGpu, NewDesGpu} = if
        (0 < DesGpu) and (DesGpu =< NodeGpu) -> 
            {lists:flatten(["@", Ip, ":gpu:", integer_to_list(NodeGpu)]), 0};
        (0 < DesGpu) and (0 < NodeGpu) -> 
            {lists:flatten(["@", Ip, ":gpu:", integer_to_list(NodeGpu)]), DesGpu - NodeGpu};
        true -> 
            {"", DesGpu}
    end,

    Reqs = [StrCpu, StrMem, StrGpu],
    ValidReqs = [R || R <- Reqs, R /= ""],
    Res = string:join(ValidReqs, " "),

    if 
        Res /= "" -> 
            case getResourcesStr(NS, {NewDesCpu, NewDesMem, NewDesGpu}) of
                "" -> Res;
                Rest -> lists:flatten([Res, " ", Rest])
            end;
        true -> 
            getResourcesStr(NS, {NewDesCpu, NewDesMem, NewDesGpu})
    end;
getResourcesStr([], _) -> erlang:error({parse_error, "ERROR: Imposible case."}).

%% ----------------------------------------------------------------------
%% Calculates the total sum of available resources across all provided nodes.
%% Parameters:
%% Nodes List of nodes in records.
%% Return: A tuple {TotalCpu, TotalMem, TotalGpu} with the aggregated values.
countRes([]) -> {0, 0, 0};
countRes([#node{ip = _, port = _, cpu = Cpu, mem = Mem, gpu = Gpu} | NS]) -> 
    {NxtCpu, NxtMem, NxtGpu} = countRes(NS),
    {Cpu + NxtCpu, Mem + NxtMem, Gpu + NxtGpu}.