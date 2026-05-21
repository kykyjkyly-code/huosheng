local KickerRole2BallDir = function ()
    return CRole2BallDir("Kicker")
end

gPlayTable.CreatePlay{
    firstState = "StateInit",

    -- 初始状态：Receiver先拿球，Kicker去指定位置准备
    ["StateInit"] = {
        switch = function ()
            if CIsGetBall("Receiver") then
                return "Set"
            end
        end,
        Kicker   = task.GotoPos("Kicker", 60, 50, KickerRole2BallDir),
        Receiver = task.GetBall("Receiver", "Kicker")
    },

    -- 准备状态：保持站位，等待一段时间后进入传球阶段
    ["Set"] = {
        switch = function ()
            if Cbuf_cnt(true, 80) then
                return "PassBall"
            end
        end,
        Kicker   = task.GotoPos("Kicker", 60, 50, KickerRole2BallDir),
        Receiver = task.ReceiverTask("GetBall_Front")
    },

    -- 传球/处理球状态
    ["PassBall"] = {
        switch = function ()
            if CBall2RoleDist("Receiver") > 30 then
                return "Delay"
            end
        end,
        Receiver = task.ReceiverTask("Shoot20"),
        Kicker   = task.RobotWait("Kicker")
    },

    -- 延时状态：给动作衔接留一点时间
    ["Delay"] = {
        switch = function ()
            if Cbuf_cnt(1, 110) then
                return "Temp"
            end
        end,
    },

    -- 临时跑位状态：两人重新移动到指定位置
    ["Temp"] = {
        switch = function ()
            if COurRole_x("Receiver") < 10 then
                return "Shoot"
            end
        end,
        Receiver = task.GotoPos("Receiver", 0, 0, 0),
        Kicker   = task.GotoPos("Kicker", 50, 50, 0)
    },

    -- 最终射门状态
    ["Shoot"] = {
        switch = function ()
            if CGetBallX() > 285 and CGetBallY() < 70 and CGetBallY() > -70 then
                return "finish"
            end
        end,
        Kicker   = task.KickerTask("Shoot"),
        Receiver = task.RobotWait("Receiver")
    },

    name = "Point123"
}