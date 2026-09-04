--[[
pleasego：3V3正常比赛战术

区域划分：
    X >= 0：前场。Kicker和Receiver中离球更近者直接拿球射门。
    X < 0 ：后场。离球更近者拿球并完成一次平传，另一台车固定用3jie接球后射门。

防守：
    对方控球时，Kicker前往对方1、2号中点封堵，Receiver退到后场辅助防守。
]]

--==================== 参数 ====================

-- 前后场分界线的X坐标。球的X>=0属于前场，直接由近球者射门；
-- 球的X<0属于后场，先完成一次传球再射门。
local FRONT_FIELD_X = 0

-- 接球队员距离其DLL任务目标点小于30时，认为接应站位基本到位。
-- 数值越小，到位要求越严格；过小可能因定位误差一直无法进入传球。
local TARGET_READY_DIST = 30

-- 当CIsGetBall偶尔漏检时使用的近球降级阈值。
-- 球员到球距离小于15可参与“已经拿到/接到球”的稳定判断；
-- 它只表示球非常近，不等同于CIsGetBall确认控球。
local BALL_CONTROL_DIST = 20

-- 传球后，球离传球队员超过10，才确认球已经真正离脚。
local PASS_LEAVE_DIST = 20

-- 射门后，球离射门队员超过10，才确认本次射门已经完成。
local SHOOT_LEAVE_DIST = 20

-- 接球或初次拿球条件需要连续成立10帧，过滤单帧视觉抖动。
-- 按60 FPS计算约为0.17秒。
local RECEIVE_STABLE_FRAMES = 30

-- 射门或传球前的稳定控球条件需要连续成立20帧。
-- 按60 FPS计算约为0.33秒。
local CONTROL_STABLE_FRAMES = 20

-- 普通动作最多等待180帧，约3秒；超时后返回judge重新判断球权。
local ACTION_TIMEOUT = 100

-- 拿球等较慢动作最多等待100帧，约5秒；超时后返回judge。
local LONG_ACTION_TIMEOUT = 300

-- 固定参与防守中点计算的对方两台场上车编号。
-- 只有1号、2号都被视觉识别为在场时，才读取坐标并计算中点；
-- 对方守门员不参与这个中点计算。
local OPP_KICKER_ID = 1
local OPP_RECEIVER_ID = 2

-- 独立的Lua状态计时器。Cbuf_cnt只用于连续条件判断，避免两个Cbuf_cnt互相干扰。
local stateElapsedFrames = {}
local activeTimedState = nil

local function tickStateTimer(stateName, timeoutFrames)
    -- 进入另一个计时状态时，自动丢弃上一状态遗留的帧数。
    if activeTimedState ~= stateName then
        for oldStateName in pairs(stateElapsedFrames) do
            stateElapsedFrames[oldStateName] = nil
        end
        activeTimedState = stateName
    end

    local elapsed = (stateElapsedFrames[stateName] or 0) + 1
    stateElapsedFrames[stateName] = elapsed
    return elapsed >= timeoutFrames
end

local function leaveState(stateName, nextState)
    stateElapsedFrames[stateName] = nil
    if activeTimedState == stateName then
        activeTimedState = nil
    end
    return nextState
end

local function resetAllStateTimers()
    for stateName in pairs(stateElapsedFrames) do
        stateElapsedFrames[stateName] = nil
    end
    activeTimedState = nil
end


--==================== 场上信息 ====================

local function ballInFrontField()
    return CGetBallX() >= FRONT_FIELD_X
end

local function kickerCloserToBall()
    -- 距离相同时默认选择Kicker，防止两台车同时抢球。
    return CBall2RoleDist("Kicker") <= CBall2RoleDist("Receiver")
end

local function opponentIds()
    local result = {}
    local nums = CGetOppNums()

    if type(nums) ~= "table" then
        return result
    end

    for key, value in pairs(nums) do
        if type(value) == "number" then
            table.insert(result, value)
        elseif value == true and type(key) == "number" then
            table.insert(result, key)
        end
    end

    return result
end

local function opponentExists(targetId)
    for _, id in ipairs(opponentIds()) do
        if id == targetId then
            return true
        end
    end

    return false
end

local function opponentPairExists()
    return opponentExists(OPP_KICKER_ID)
        and opponentExists(OPP_RECEIVER_ID)
end

local function opponentHasBall()
    for _, id in ipairs(opponentIds()) do
        if COppIsGetBall(id) then
            return true
        end
    end

    return false
end

local function opponentMidX()
    return (COppNum_x(OPP_KICKER_ID) + COppNum_x(OPP_RECEIVER_ID)) / 2
end

local function opponentMidY()
    return (COppNum_y(OPP_KICKER_ID) + COppNum_y(OPP_RECEIVER_ID)) / 2
end

local function kickerToBallDir()
    return CRole2BallDir("Kicker")
end

local function defenceInterrupt()
    if opponentHasBall() then
        resetAllStateTimers()

        if opponentPairExists() then
            return "defend"
        end

        return "defend_fallback"
    end

    return nil
end

local function selectAttackState()
    if ballInFrontField() then
        if kickerCloserToBall() then
            return "front_kicker_get"
        end

        return "front_receiver_get"
    end

    if kickerCloserToBall() then
        return "back_kicker_get"
    end

    return "back_receiver_get"
end


--==================== 战术状态机 ====================

gPlayTable.CreatePlay{

firstState = "judge",

-- 总控：对方控球优先防守，否则根据球区和距离选择进攻入口。
["judge"] = {
    switch = function()
        resetAllStateTimers()

        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        return "choose"
    end,
    Kicker = task.RobotHalt("Kicker"),
    Receiver = task.RobotHalt("Receiver"),
    Goalie = task.GoalieTask("smartgoalie"),
},

-- X=0分区；前后场都由离球更近的场上车处理球。
["choose"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        return selectAttackState()
    end,
    Kicker = task.RobotHalt("Kicker"),
    Receiver = task.RobotHalt("Receiver"),
    Goalie = task.GoalieTask("smartgoalie"),
},

-- 对方1、2号齐全：Kicker前往两车中点，Receiver回后场辅助防守。
["defend"] = {
    switch = function()
        if not opponentHasBall() then
            return "judge"
        end

        if not opponentPairExists() then
            return "defend_fallback"
        end
    end,
    Kicker = task.GotoPos(
        "Kicker", opponentMidX, opponentMidY, kickerToBallDir
    ),
    Receiver = task.GotoPos("Receiver", -250, 0, 0),
    Goalie = task.GoalieTask("smartgoalie"),
},

-- 对方编号不完整时不读取无效坐标，等待视觉恢复或球权变化。
["defend_fallback"] = {
    switch = function()
        local stateName = "defend_fallback"
        local timedOut = tickStateTimer(stateName, ACTION_TIMEOUT)

        if not opponentHasBall() then
            return leaveState(stateName, "judge")
        end

        if opponentPairExists() then
            return leaveState(stateName, "defend")
        end

        if timedOut then
            return leaveState(stateName, "judge")
        end
    end,
    Kicker = task.RobotHalt("Kicker"),
    Receiver = task.GotoPos("Receiver", -250, 0, 0),
    Goalie = task.GoalieTask("smartgoalie"),
},


--==================== 前场：近球者直接射门 ====================

["front_kicker_get"] = {
    switch = function()
        local stateName = "front_kicker_get"

        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return leaveState(stateName, defendState)
        end

        local timedOut = tickStateTimer(stateName, ACTION_TIMEOUT)

        local controlled = CIsGetBall("Kicker")
            or (CBall2RoleDist("Kicker") < BALL_CONTROL_DIST)
        if Cbuf_cnt(controlled, RECEIVE_STABLE_FRAMES) then
            return leaveState(stateName, "front_kicker_settle")
        end

        if timedOut then
            return leaveState(stateName, "judge")
        end
    end,
    Kicker = task.KickerTask("3sget"),
    Receiver = task.RobotHalt("Receiver"),
    Goalie = task.GoalieTask("smartgoalie"),
},

["front_kicker_settle"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "front_kicker_settle", ACTION_TIMEOUT
        )

        local controlled = CIsGetBall("Kicker")
            or (CBall2RoleDist("Kicker") < BALL_CONTROL_DIST)
        if Cbuf_cnt(controlled, 0) then
            return "front_kicker_shoot"
        end

        if timedOut then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("newshoot"),
    Receiver = task.RobotHalt("Receiver"),
    Goalie = task.GoalieTask("smartgoalie"),
},

["front_kicker_shoot"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "front_kicker_shoot", ACTION_TIMEOUT
        )

        local shotFinished = CIsBallKick("Kicker")
            and (CBall2RoleDist("Kicker") > SHOOT_LEAVE_DIST)
        if shotFinished or timedOut then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("2shootping"),
    Receiver = task.RobotHalt("Receiver"),
    Goalie = task.GoalieTask("smartgoalie"),
},

["front_receiver_get"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "front_receiver_get", LONG_ACTION_TIMEOUT
        )

        local controlled = CIsGetBall("Receiver")
            or (CBall2RoleDist("Receiver") < BALL_CONTROL_DIST)
        if Cbuf_cnt(controlled, RECEIVE_STABLE_FRAMES) then
            return "front_receiver_settle"
        end

        if timedOut then
            return "judge"
        end
    end,
    Kicker = task.RobotHalt("Kicker"),
    Receiver = task.ReceiverTask("1pget"),
    Goalie = task.GoalieTask("smartgoalie"),
},

["front_receiver_settle"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "front_receiver_settle", ACTION_TIMEOUT
        )

        local controlled = CIsGetBall("Receiver")
            or (CBall2RoleDist("Receiver") < BALL_CONTROL_DIST)
        if Cbuf_cnt(controlled, CONTROL_STABLE_FRAMES) then
            return "front_receiver_shoot"
        end

        if timedOut then
            return "judge"
        end
    end,
    Kicker = task.RobotHalt("Kicker"),
    Receiver = task.ReceiverTask("3sget"),
    Goalie = task.GoalieTask("smartgoalie"),
},

["front_receiver_shoot"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "front_receiver_shoot", ACTION_TIMEOUT
        )

        local shotFinished = CIsBallKick("Receiver")
            and (CBall2RoleDist("Receiver") > SHOOT_LEAVE_DIST)
        if shotFinished or timedOut then
            return "judge"
        end
    end,
    Kicker = task.RobotHalt("Kicker"),
    Receiver = task.ReceiverTask("2shootping"),
    Goalie = task.GoalieTask("smartgoalie"),
},


--==================== 后场：近球者拿球，一次传球 ====================

-- Kicker先拿球，Receiver从准备到接球始终使用3jie。
["back_kicker_get"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "back_kicker_get", LONG_ACTION_TIMEOUT
        )

        local ready = (CIsGetBall("Kicker")
                or (CBall2RoleDist("Kicker") < BALL_CONTROL_DIST))
            and (CRole2TargetDist("Receiver") < TARGET_READY_DIST)
        if Cbuf_cnt(ready, RECEIVE_STABLE_FRAMES) then
            return "back_kicker_settle"
        end

        if timedOut then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("1pget"),
    Receiver = task.ReceiverTask("3jie"),
    Goalie = task.GoalieTask("smartgoalie"),
},

["back_kicker_settle"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "back_kicker_settle", ACTION_TIMEOUT
        )

        local ready = CIsGetBall("Kicker")
            and (CRole2TargetDist("Receiver") < TARGET_READY_DIST)
        if Cbuf_cnt(ready, CONTROL_STABLE_FRAMES) then
            return "back_kicker_pass"
        end

        if timedOut then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("1pget"),
    Receiver = task.ReceiverTask("3jie"),
    Goalie = task.GoalieTask("smartgoalie"),
},

["back_kicker_pass"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "back_kicker_pass", ACTION_TIMEOUT
        )

        local passFinished = CIsBallKick("Kicker")
            and (CBall2RoleDist("Kicker") > PASS_LEAVE_DIST)
        if passFinished then
            return "back_receiver_receive"
        end

        if timedOut then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("5pass"),
    Receiver = task.ReceiverTask("3jie"),
    Goalie = task.GoalieTask("smartgoalie"),
},

["back_receiver_receive"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "back_receiver_receive", ACTION_TIMEOUT
        )

        local received = CIsGetBall("Receiver")
            or (CBall2RoleDist("Receiver") < BALL_CONTROL_DIST)
        if Cbuf_cnt(received, RECEIVE_STABLE_FRAMES) then
            return "front_receiver_settle"
        end

        if timedOut then
            return "judge"
        end
    end,
    Kicker = task.RobotHalt("Kicker"),
    Receiver = task.ReceiverTask("3jie"),
    Goalie = task.GoalieTask("smartgoalie"),
},

-- Receiver先拿球，Kicker从准备到接球始终使用3jie。
["back_receiver_get"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "back_receiver_get", LONG_ACTION_TIMEOUT
        )

        local ready = (CIsGetBall("Receiver")
                or (CBall2RoleDist("Receiver") < BALL_CONTROL_DIST))
            and (CRole2TargetDist("Kicker") < TARGET_READY_DIST)
        if Cbuf_cnt(ready, RECEIVE_STABLE_FRAMES) then
            return "back_receiver_settle"
        end

        if timedOut then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.ReceiverTask("1pget"),
    Goalie = task.GoalieTask("smartgoalie"),
},

["back_receiver_settle"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "back_receiver_settle", ACTION_TIMEOUT
        )

        local ready = CIsGetBall("Receiver")
            and (CRole2TargetDist("Kicker") < TARGET_READY_DIST)
        if Cbuf_cnt(ready, CONTROL_STABLE_FRAMES) then
            return "back_receiver_pass"
        end

        if timedOut then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.ReceiverTask("1pget"),
    Goalie = task.GoalieTask("smartgoalie"),
},

["back_receiver_pass"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "back_receiver_pass", ACTION_TIMEOUT
        )

        local passFinished = CIsBallKick("Receiver")
            and (CBall2RoleDist("Receiver") > PASS_LEAVE_DIST)
        if passFinished then
            return "back_kicker_receive"
        end

        if timedOut then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.ReceiverTask("5pass"),
    Goalie = task.GoalieTask("smartgoalie"),
},

["back_kicker_receive"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local timedOut = tickStateTimer(
            "back_kicker_receive", ACTION_TIMEOUT
        )

        local received = CIsGetBall("Kicker")
            or (CBall2RoleDist("Kicker") < BALL_CONTROL_DIST)
        if Cbuf_cnt(received, RECEIVE_STABLE_FRAMES) then
            return "front_kicker_settle"
        end

        if timedOut then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.RobotHalt("Receiver"),
    Goalie = task.GoalieTask("smartgoalie"),
},

name = "pleasego"

}
