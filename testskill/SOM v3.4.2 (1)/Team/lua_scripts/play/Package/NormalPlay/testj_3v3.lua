-- 3V3 正常比赛攻防战术
--
-- 角色分工：
--   Kicker、Receiver 为两台场上机器人；Goalie 为守门员。
-- 防守原则：
--   离球较近的场上机器人封堵敌方 1、2 号之间的传球线路，
--   另一台机器人执行 NormalDef，封堵球到我方球门的线路。
-- 进攻原则：
--   保留 testj.lua 原有的前场一传一射和后场两次传球流程。

--==================== 可调参数 ====================

local BACK_FIELD_X = -100

local TARGET_READY_DIST = 30
local GET_BALL_DIST = 25
local PASS_LEAVE_DIST = 20
local SHOOT_LEAVE_DIST = 30

local STABLE_FRAMES = 20
local RECEIVE_STABLE_FRAMES = 10
local ACTION_TIMEOUT = 180
local LONG_ACTION_TIMEOUT = 300

local OPP_KICKER_ID = 1
local OPP_RECEIVER_ID = 2

--==================== 比赛信息辅助函数 ====================

local function ballInBackField()
    return CGetBallX() < BACK_FIELD_X
end

local function ourBallOwner()
    if CIsGetBall("Kicker") then
        return "Kicker"
    end

    if CIsGetBall("Receiver") then
        return "Receiver"
    end

    return nil
end

-- CGetOppNums() 按手册返回敌方在场车号表。
-- 兼容“车号作为表值”和“车号作为键、值为 true”两种常见形式。
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
    local ids = opponentIds()

    for _, id in ipairs(ids) do
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
    local ids = opponentIds()

    for _, id in ipairs(ids) do
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

local function receiverToBallDir()
    return CRole2BallDir("Receiver")
end

local function kickerCloserToBall()
    return CBall2RoleDist("Kicker") <= CBall2RoleDist("Receiver")
end

--==================== 状态选择辅助函数 ====================

local function selectDefenceState()
    if not opponentPairExists() then
        return "defend_fallback"
    end

    if kickerCloserToBall() then
        return "defend_kicker_mid"
    end

    return "defend_receiver_mid"
end

local function selectLooseBallState()
    if kickerCloserToBall() then
        if ballInBackField() then
            return "back_kicker_get"
        end

        return "front_kicker_get"
    end

    if ballInBackField() then
        return "back_receiver_get"
    end

    return "front_receiver_get"
end

local function selectOurAttackState()
    local owner = ourBallOwner()

    if owner == "Kicker" then
        if ballInBackField() then
            return "back_kicker_settle"
        end

        return "front_kicker_settle"
    end

    if owner == "Receiver" then
        if ballInBackField() then
            return "back_receiver_settle"
        end

        return "front_receiver_settle"
    end

    return selectLooseBallState()
end

local function selectMainState()
    if opponentHasBall() then
        return selectDefenceState()
    end

    if ourBallOwner() ~= nil then
        return selectOurAttackState()
    end

    return selectLooseBallState()
end

local function defenceInterrupt()
    if opponentHasBall() then
        return selectDefenceState()
    end

    return nil
end

--==================== 战术状态机 ====================

gPlayTable.CreatePlay{

firstState = "judge",

-- 总控状态：每轮进攻结束、动作失败或球权变化后，都回到这里重新判断。
["judge"] = {
    switch = function()
        return selectMainState()
    end,
    Kicker = task.RobotHalt("Kicker"),
    Receiver = task.RobotHalt("Receiver"),
    Goalie = task.GoalieTask("lashoumen"),
},

--==================== 防守与自由球 ====================

-- Kicker 距球更近：Kicker 卡敌方 1、2 号中点，Receiver 封堵球门线路。
["defend_kicker_mid"] = {
    switch = function()
        if not opponentHasBall() then
            return "judge"
        end

        if not opponentPairExists() then
            return "defend_fallback"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.GotoPos(
        "Kicker", opponentMidX, opponentMidY, kickerToBallDir
    ),
    Receiver = task.NormalDef("Receiver"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Receiver 距球更近：Receiver 卡敌方 1、2 号中点，Kicker 封堵球门线路。
["defend_receiver_mid"] = {
    switch = function()
        if not opponentHasBall() then
            return "judge"
        end

        if not opponentPairExists() then
            return "defend_fallback"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.NormalDef("Kicker"),
    Receiver = task.GotoPos(
        "Receiver", opponentMidX, opponentMidY, receiverToBallDir
    ),
    Goalie = task.GoalieTask("lashoumen"),
},

-- 敌方 1、2 号信息不完整时，不读取无效坐标。
-- Kicker 执行普通防守，Receiver 停车，避免两车争抢同一个 NormalDef 目标点。
["defend_fallback"] = {
    switch = function()
        if not opponentHasBall() then
            return "judge"
        end

        if opponentPairExists() then
            return selectDefenceState()
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.NormalDef("Kicker"),
    Receiver = task.RobotHalt("Receiver"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- 中前场自由球：Kicker 离球更近时由 Kicker 拿球，Receiver 前往接应位置。
["front_kicker_get"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        if CIsGetBall("Kicker") then
            return "front_kicker_settle"
        end

        if CIsGetBall("Receiver") then
            return selectOurAttackState()
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("pget10"),
    Receiver = task.ReceiverTask("3jie"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- 后场自由球：Kicker 离球更近时由 Kicker 拿球，Receiver 前往二次接球点。
["back_kicker_get"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        if CIsGetBall("Kicker") then
            return "back_kicker_settle"
        end

        if CIsGetBall("Receiver") then
            return selectOurAttackState()
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("pget10"),
    Receiver = task.ReceiverTask("3jie2ci"),
    Goalie = task.GoalieTask("lashoumen"),
},

--==================== 中前场进攻 ====================

-- Receiver 拿球，Kicker 运行到接应位置。
["front_receiver_get"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        if CIsGetBall("Kicker") then
            return selectOurAttackState()
        end

        local ready = (CRole2TargetDist("Kicker") < 20)
            and (CBall2RoleDist("Receiver") < 20)
        if Cbuf_cnt(ready, RECEIVE_STABLE_FRAMES) then
            return "front_receiver_settle"
        end

        if Cbuf_cnt(true, LONG_ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.ReceiverTask("pget10"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Receiver 稳定控球后准备传给 Kicker。
["front_receiver_settle"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        if CIsGetBall("Kicker") then
            return selectOurAttackState()
        end

        if Cbuf_cnt(CIsGetBall("Receiver"), STABLE_FRAMES) then
            return "front_pass"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.ReceiverTask("accpget7"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Receiver 传球给 Kicker。
["front_pass"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local passFinished = CIsBallKick("Receiver")
            and (CBall2RoleDist("Receiver") > PASS_LEAVE_DIST)
        if passFinished then
            return "front_kicker_receive"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.ReceiverTask("pass0.15"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Kicker 接球，Receiver 离开球路并进入防守位置。
["front_kicker_receive"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local received = CIsGetBall("Kicker")
            or (CBall2RoleDist("Kicker") < TARGET_READY_DIST)
        if Cbuf_cnt(received, RECEIVE_STABLE_FRAMES) then
            return "front_kicker_settle"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.GotoPos("Receiver", -170, -100, 0),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Kicker 调整射门姿态。
["front_kicker_settle"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local controlled = CIsGetBall("Kicker")
            or (CBall2RoleDist("Kicker") < 50)
        if Cbuf_cnt(controlled, STABLE_FRAMES) then
            return "front_shoot"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3sget"),
    Receiver = task.ReceiverTask("3jie"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Kicker 平射；射门后重新判断球权，正常比赛不会结束 play。
["front_shoot"] = {
    switch = function()
        local shotFinished = CIsBallKick("Kicker")
            and (CBall2RoleDist("Kicker") > SHOOT_LEAVE_DIST)
        if shotFinished or Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("shootpings"),
    Receiver = task.NormalDef("Receiver"),
    Goalie = task.GoalieTask("lashoumen"),
},

--==================== 后场进攻 ====================

-- Receiver 在后场拿球，Kicker 准备第一次接球。
["back_receiver_get"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        if CIsGetBall("Kicker") then
            return "back_kicker_settle"
        end

        local ready = (CRole2TargetDist("Kicker") < GET_BALL_DIST)
            and (CBall2RoleDist("Receiver") < GET_BALL_DIST)
        if Cbuf_cnt(ready, RECEIVE_STABLE_FRAMES) then
            return "back_receiver_settle"
        end

        if Cbuf_cnt(true, LONG_ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.ReceiverTask("pget10"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Receiver 稳定控球后准备第一次传球。
["back_receiver_settle"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        if CIsGetBall("Kicker") then
            return "back_kicker_settle"
        end

        if Cbuf_cnt(CIsGetBall("Receiver"), STABLE_FRAMES) then
            return "back_first_pass"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.ReceiverTask("accpget7"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Receiver 第一次传球给 Kicker。
["back_first_pass"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local passFinished = CIsBallKick("Receiver")
            and (CBall2RoleDist("Receiver") > PASS_LEAVE_DIST)
        if passFinished then
            return "back_kicker_receive"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.ReceiverTask("pass0.15"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Kicker 接第一次传球，Receiver 前往第二次接球位置。
["back_kicker_receive"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local ready = CIsGetBall("Kicker")
            and (CRole2TargetDist("Receiver") < TARGET_READY_DIST)
        if Cbuf_cnt(ready, STABLE_FRAMES) then
            return "back_kicker_settle"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("3jie"),
    Receiver = task.ReceiverTask("3jie2ci"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Kicker 稳定控球，并等待 Receiver 到达第二次接球位置。
["back_kicker_settle"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local ready = CIsGetBall("Kicker")
            and (CRole2TargetDist("Receiver") < TARGET_READY_DIST)
        if Cbuf_cnt(ready, STABLE_FRAMES) then
            return "back_second_pass"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("accpget7"),
    Receiver = task.ReceiverTask("3jie2ci"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Kicker 第二次传球给 Receiver。
["back_second_pass"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local passFinished = CIsBallKick("Kicker")
            and (CBall2RoleDist("Kicker") > PASS_LEAVE_DIST)
        if passFinished then
            return "back_receiver_receive"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.KickerTask("pass0.15"),
    Receiver = task.ReceiverTask("3jie"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Receiver 接第二次传球。
["back_receiver_receive"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local received = CIsGetBall("Receiver")
            or (CBall2RoleDist("Receiver") < TARGET_READY_DIST)
        if Cbuf_cnt(received, RECEIVE_STABLE_FRAMES) then
            return "back_receiver_shoot_settle"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.NormalDef("Kicker"),
    Receiver = task.ReceiverTask("3jie2ci"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Receiver 调整射门姿态。
["back_receiver_shoot_settle"] = {
    switch = function()
        local defendState = defenceInterrupt()
        if defendState ~= nil then
            return defendState
        end

        local controlled = CIsGetBall("Receiver")
            or (CBall2RoleDist("Receiver") < 50)
        if Cbuf_cnt(controlled, STABLE_FRAMES) then
            return "back_shoot"
        end

        if Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.NormalDef("Kicker"),
    Receiver = task.ReceiverTask("3sget"),
    Goalie = task.GoalieTask("lashoumen"),
},

-- Receiver 平射；射门后返回总控状态继续比赛。
["back_shoot"] = {
    switch = function()
        local shotFinished = CIsBallKick("Receiver")
            and (CBall2RoleDist("Receiver") > SHOOT_LEAVE_DIST)
        if shotFinished or Cbuf_cnt(true, ACTION_TIMEOUT) then
            return "judge"
        end
    end,
    Kicker = task.NormalDef("Kicker"),
    Receiver = task.ReceiverTask("shootpings"),
    Goalie = task.GoalieTask("lashoumen"),
},

name = "testj_3v3"

}
