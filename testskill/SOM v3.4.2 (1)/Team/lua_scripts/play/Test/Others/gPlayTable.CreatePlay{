gPlayTable.CreatePlay{

    firstState = "start",

    ["start"] = {
        switch = function()
            return "getball"
        end,

        Receiver = task.GetBall("Receiver", "Kicker"),
        Kicker   = task.GotoPos("Kicker", 100, 80, 0),
    },

    ["getball"] = {
        switch = function()
            -- Receiver 真正拿到球，再进入传球
            if CIsGetBall("Receiver") then
                return "passball"
            end
        end,

        Receiver = task.GetBall("Receiver", "Kicker"),
        Kicker   = task.GotoPos("Kicker", 100, 80, 0),
    },

    ["passball"] = {
        switch = function()
            -- Kicker 真正控到球，再去射门
            if CIsGetBall("Kicker") then
                return "shoot"
            end

            -- 如果 Receiver 丢球了，且 Kicker 也没拿到，回去重新抢球
            if (not CIsGetBall("Receiver")) and CBall2RoleDist("Kicker") > 30 then
                return "getball"
            end
        end,

        Receiver = task.PassBall("Receiver", "Kicker"),
        Kicker   = task.ReceiveBall("Kicker"),
    },

    ["shoot"] = {
        switch = function()
            -- 确认 Kicker 已经完成踢球动作
            if CIsBallKick("Kicker") then
                return "finish"
            end
        end,

        Receiver = task.Stop("Receiver", 1),
        Kicker   = task.Shoot("Kicker"),
    },

    ["finish"] = {
        switch = function()
        end,

        Receiver = task.Stop("Receiver", 1),
        Kicker   = task.Stop("Kicker", 2),
    },

    name = "test_optimized"
}