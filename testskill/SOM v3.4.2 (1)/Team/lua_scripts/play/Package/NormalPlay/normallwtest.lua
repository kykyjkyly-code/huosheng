gPlayTable.CreatePlay{

firstState = "initState",

-- 初始状态：判断谁已经拿球，或谁离球更近
["initState"] = {
    switch = function()
        if CIsGetBall("Kicker") then
            return "passToReceiver"
        elseif CIsGetBall("Receiver") then
            return "receiverPassToKicker"
        elseif CBall2RoleDist("Kicker") <= CBall2RoleDist("Receiver") then
            return "kickerGetBall"
        else
            return "receiverGetBall"
        end
    end,

    Kicker   = task.GetBall("Kicker", "Receiver"),
    -- Receiver = task.ReceiverTask("def"),
    Receiver = task.NormalDef("Receiver"),
    Goalie   = task.Goalie()
},

-- Kicker 离球近：Kicker 先拿球，Receiver 去接球点
["kickerGetBall"] = {
    switch = function()
        if CIsGetBall("Kicker") and CRole2TargetDist("Receiver") < 5 then
            return "passToReceiver"
        end
    end,

    Kicker   = task.GetBall("Kicker", "Receiver"),
    Receiver = task.GoRecePos("Receiver"),
    Goalie   = task.Goalie()
},

-- Receiver 离球近：Receiver 先拿球，Kicker 去接球点
["receiverGetBall"] = {
    switch = function()
        if CIsGetBall("Receiver") and CRole2TargetDist("Kicker") < 5 then
            return "receiverPassToKicker"
        end
    end,

    Kicker   = task.GoRecePos("Kicker"),
    Receiver = task.GetBall("Receiver", "Kicker"),
    Goalie   = task.Goalie()
},

-- Receiver 先传给 Kicker，把球权交给 Kicker
["receiverPassToKicker"] = {
    switch = function()
        if CIsBallKick("Receiver") then
            return "kickerReceiveFirstBall"
        end
    end,

    Kicker   = task.GoRecePos("Kicker"),
    Receiver = task.PassBall("Receiver", "Kicker"),
    Goalie   = task.Goalie()
},

-- Kicker 接 Receiver 的第一脚传球
["kickerReceiveFirstBall"] = {
    switch = function()
        if CIsGetBall("Kicker") and CRole2TargetDist("Receiver") < 5 then
            return "passToReceiver"
        end
    end,

    Kicker   = task.ReceiveBall("Kicker"),
    Receiver = task.GoRecePos("Receiver"),
    Goalie   = task.Goalie()
},

-- 第一次正式配合：Kicker 传给 Receiver
["passToReceiver"] = {
    switch = function()
        if CIsBallKick("Kicker") then
            return "receiverReceiveBall"
        end
    end,

    Kicker   = task.PassBall("Kicker", "Receiver"),
    Receiver = task.GoRecePos("Receiver"),
    Goalie   = task.Goalie()
},

-- Receiver 接 Kicker 的传球
["receiverReceiveBall"] = {
    switch = function()
        if CIsGetBall("Receiver") and CRole2TargetDist("Kicker") < 5 then
            return "backPassToKicker"
        end
    end,

    Kicker   = task.GoRecePos("Kicker"),
    Receiver = task.ReceiveBall("Receiver"),
    Goalie   = task.Goalie()
},

-- 第二次配合：Receiver 回传给 Kicker
["backPassToKicker"] = {
    switch = function()
        if CIsBallKick("Receiver") then
            return "kickerReceiveBackBall"
        end
    end,

    Kicker   = task.GoRecePos("Kicker"),
    Receiver = task.PassBall("Receiver", "Kicker"),
    Goalie   = task.Goalie()
},

-- Kicker 接 Receiver 的回传球
["kickerReceiveBackBall"] = {
    switch = function()
        if CIsGetBall("Kicker") then
            return "shoot"
        end
    end,

    Kicker   = task.ReceiveBall("Kicker"),
    -- Receiver = task.ReceiverTask("def"),
    Receiver = task.NormalDef("Receiver"),
    Goalie   = task.Goalie()
},

-- Kicker 射门
["shoot"] = {
    switch = function()
        if CIsBallKick("Kicker") or Cbuf_cnt(true, 120) then
            return "initState"
        end
    end,

    Kicker   = task.Shoot("Kicker"),
    -- Receiver = task.ReceiverTask("def"),
    Receiver = task.NormalDef("Receiver"),
    Goalie   = task.Goalie()
},

name = "NormalPlayDefend"

}