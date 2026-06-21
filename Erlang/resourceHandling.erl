-module(resourceHandling).
-export([parseNodesStr/1, getNodeRes/2, toRecord/1, getResourcesStr/2, createReqData/1, countRes/1]).
-include("globals.hrl").


parseNodesStr(NodesData) ->
    % NodesData => "Node;Node..." where Node => ip:port:cpu:x:mem:y:gpu:z
    Nodes = string:tokens(NodesData, ";"), % string:lexemes
    NodesParsed = lists:map(fun(X) -> string:tokens(X, ":") end, Nodes),
    lists:map(fun(X) -> toRecord(X) end, NodesParsed). % => [#node{ip, port, cpu, mem, gpu}, ...]


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


getNodeRes([], Map) -> 
    Map;
getNodeRes(["cpu", Val | Rest], Map) ->
    getNodeRes(Rest, maps:put(cpu, list_to_integer(Val), Map));
getNodeRes(["mem", Val | Rest], Map) ->
    getNodeRes(Rest, maps:put(mem, list_to_integer(Val), Map));
getNodeRes(["gpu", Val | Rest], Map) ->
    getNodeRes(Rest, maps:put(gpu, list_to_integer(Val), Map));
getNodeRes(_, _) -> erlang:error({format_error, "ERROR: Invalid format."}).


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


countRes([]) -> {0, 0, 0};
countRes([#node{ip = _, port = _, cpu = Cpu, mem = Mem, gpu = Gpu} | NS]) -> 
    {NxtCpu, NxtMem, NxtGpu} = countRes(NS),
    {Cpu + NxtCpu, Mem + NxtMem, Gpu + NxtGpu}.