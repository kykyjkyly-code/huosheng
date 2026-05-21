--

local r = function()
    return -30
end
local min = function()
    return -200
end

local max = function()
    return 200
end

local ballx = function()
    return CGetBallX() +100
end

local bally = function()
    return CGetBallY() +100
end
local bally1 = function()
    return bally()+80 
end

local ballfordoor = function()
    return CBall2PointDir(300, 0)
end

local Kickerdir2Receiverdir = function()
    return COurRole2RoleDir("Kicker", "Receiver")
end

local rdk = function()
    return COurRole2RoleDir("Receiver", "Kicker")
end
-- 判断是否进球/球进入球门附近
local isGoal = function()
    return CGetBallballx> 285 and CGetBallbally()< 70 and CGetBallbally()> -70
end

-- 判断球是否还在可抢范围内
-- 这里范围你可以按场地改，小场地一般 x/y 不要太大
local ballInField = function()
    return CGetBallballx< 285 and CGetBallballx> -300 and CGetBallbally()< 200 and CGetBallbally()> -200
end

-- 判断 Kicker 更靠近球
local kickerNearerballx= function()
    return CBall2RoleDist("Kicker") <= CBall2RoleDist("Receiver")
end
-- ========= 新增：接球点计算 =========

-- 防止接球点跑出边界，数值可以按你的场地再微调
local receX = function()
    if ballx() < -200 then
        return -200
    elseif bally()> 200 then
        return 200
    else
        return bally()
    end
end
local receY = function()
    if bally() < -200 then
        return -200
    elseif bally()> 200 then
        return 200
    else
        return bally()
    end
end
-- 判断是否后半场：以 x = 0 为分界线
-- 我方进攻方向默认是 +x，也就是往 x = 300 的球门进攻
local isBackHalf = function()
    return ballx() < 0
end

-- 根据当前球/持球队员位置计算接球点 X
-- 因为持球时球基本贴着机器人，所以用球的位置近似持球队员位置
---local receX = function()
        -- 后场可以多往前送一点，前场别太靠近禁区
  ---  if isBackHalf() then
   ---     return clamp(ballx)
   --- else
    ---    return clamp(ballx+ 100, -200,200)
  ---  end
--
-- 根据当前球/持球队员位置计算接球点 Y
-- 默认斜上方：y + 80
-- 如果已经很靠上边线，就改成斜下方，防止出界
---local receY = function()
    ---local y = CGetBallY()

   --- if bally() > 100 then
    ---    return clamp(bally - 80, -200, 200)
    ---else
     ---   return clamp(bally + 80, -200, 200)
  ---  end
---end


gPlayTable.CreatePlay{

firstState = "judge",

-- ========= 入口：根据球的位置选择前场/后场策略 =========
["judge"] = {
    switch = function()
        if isBackHalf() then
            return "back_getball1"
        else
            return "front_getball"
        end
    end,

    Kicker = task.RobotHalt("Kicker"),
    Receiver = task.RobotHalt("Receiver"),
    Goalie = task.Goalie()
},

-- =========================================================
-- 前半场策略：传 1 次
-- Receiver 拿球 -> Kicker 斜上方接球 -> Kicker 射门
-- =========================================================

["front_getball"] = {
    switch = function()
        if (CRole2TargetDist("Kicker") < 8 and CBall2RoleDist("Receiver") < 10) 
            or Cbuf_cnt(true, 180) then
           --- return "front_forpass"
        end
    end,

    Receiver = task.GetBall("Receiver","Kicker"),
    Kicker = task.GotoPos("Kicker", receX, receY, Kickerdir2Receiverdir),
   --- Kicker = GotoPos()
    Goalie = task.Goalie()
},

["front_forpass"] = {
    switch = function()
        if Cbuf_cnt(true, 80) then
            return "front_pass"
        end
    end,

    -- Receiver 控球并准备传给 Kicker
    Receiver = task.GetBall("Receiver", "Kicker"),
    Kicker = task.GotoPos("Kicker", receX, receY, Kickerdir2Receiverdir)(),
    Goalie = task.Goalie()
},

["front_pass"] = {
    switch = function()
        if (CIsBallKick("Receiver") and CBall2RoleDist("Receiver") > 10)
            or Cbuf_cnt(true, 120) then
            return "front_wait_kicker"
        end
    end,

    -- 传球函数不变
    Receiver = task.ReceiverTask("mypassball"),
    Kicker = task.GotoPos("Kicker", receX, receY, Kickerdir2Receiverdir)(),
    Goalie = task.Goalie()
},

["front_wait_kicker"] = {
    switch = function()
        if CBall2RoleDist("Kicker") < 20 then
            return "Kickerforshoot"
        end
    end,

    Kicker = task.GetBall("Kicker", "Kicker"),
    Receiver = task.GotoPos("Receiver", 150, 0, 0),
    Goalie = task.Goalie()
},

["Kickerforshoot"] = {
    switch = function()
        if Cbuf_cnt(true, 80) then
            return "shoot_by_kicker"
        end
    end,

    Kicker = task.GetBall("Kicker", "Kicker"),
    Receiver = task.GotoPos("Receiver", 150, 0, 0),
    Goalie = task.Goalie()
},

["shoot_by_kicker"] = {
    switch = function()
        if CGetBallballx> 285 and CGetBallbally()< 70 and CGetBallbally()> -70 then
            return "deidai"
        end
    end,

    Kicker = task.Shoot("Kicker"),
    Receiver = task.GotoPos("Receiver", 150, 0, 0),
    Goalie = task.Goalie()
},


-- =========================================================
-- 后半场策略：传 2 次
-- Receiver 拿球 -> Kicker 接第一脚 -> Receiver 接第二脚 -> Receiver 射门
-- =========================================================

["back_getball1"] = {
    switch = function()
        if (CRole2TargetDist("Kicker") < 8 and CBall2RoleDist("Receiver") < 10)
            or Cbuf_cnt(true, 200) then
            return "back_forpass1"
        end
    end,

    Receiver = task.GetBall("Receiver"),
    Kicker = task.GotoPos("Kicker", receX, receY, Kickerdir2Receiverdir)(),
    Goalie = task.Goalie()
},

["back_forpass1"] = {
    switch = function()
        if Cbuf_cnt(true, 80) then
            return "back_pass1"
        end
    end,

    -- 第一脚：Receiver -> Kicker
    Receiver = task.GetBall("Receiver", "Kicker"),
    Kicker = task.GotoPos("Kicker", receX, receY, Kickerdir2Receiverdir)(),
    Goalie = task.Goalie()
},

["back_pass1"] = {
    switch = function()
        if (CIsBallKick("Receiver") and CBall2RoleDist("Receiver") > 10)
            or Cbuf_cnt(true, 120) then
            return "back_wait_kicker"
        end
    end,

    -- 传球函数不变
    Receiver = task.ReceiverTask("mypassball"),
    Kicker = task.GotoPos("Kicker", receX, receY, Kickerdir2Receiverdir)(),
    Goalie = task.Goalie()
},

["back_wait_kicker"] = {
    switch = function()
        if CBall2RoleDist("Kicker") < 20 then
            return "back_prepare_pass2"
        end
    end,

    Kicker = task.GetBall("Kicker", "Kicker"),
    Receiver = task.GotoPos("Receiver", receX, receY, rdk)(),
    Goalie = task.Goalie()
},

["back_prepare_pass2"] = {
    switch = function()
        if (CRole2TargetDist("Receiver") < 8 and CBall2RoleDist("Kicker") < 12)
            or Cbuf_cnt(true, 160) then
            return "back_forpass2"
        end
    end,

    -- 第二次接球点：根据 Kicker 当前持球位置，也就是当前球位置重新计算
    Kicker = task.GetBall("Kicker", "Receiver"),
    Receiver = task.GotoPos("Receiver", receX, receY, rdk)(),
    Goalie = task.Goalie()
},

["back_forpass2"] = {
    switch = function()
        if Cbuf_cnt(true, 10) then
            return "back_pass2"
        end
    end,

    -- 第二脚：Kicker -> Receiver
    Kicker = task.GetBall("Kicker", "Receiver"),
    Receiver = task.GotoPos("Receiver", receX, receY, rdk)(),
    Goalie = task.Goalie()
},

["back_pass2"] = {
    switch = function()
        if (CIsBallKick("Kicker") and CBall2RoleDist("Kicker") > 10)
            or Cbuf_cnt(true, 120) then
            return "back_wait_receiver"
        end
    end,

    -- 传球函数不变，只是这次执行传球动作的是 Kicker
    Kicker = task.ReceiverTask("mypassball"),
    Receiver = task.GotoPos("Receiver", receX, receY, rdk)(),
    Goalie = task.Goalie()
},

["back_wait_receiver"] = {
    switch = function()
        if CBall2RoleDist("Receiver") < 20 then
            return "Receiverforshoot"
        end
    end,

    Receiver = task.GetBall("Receiver", "Receiver"),
    Kicker = task.GotoPos("Kicker", 150, -30, 0),
    Goalie = task.Goalie()
},

["Receiverforshoot"] = {
    switch = function()
        if Cbuf_cnt(true, 80) then
            return "shoot_by_receiver"
        end
    end,

    Receiver = task.GetBall("Receiver", "Receiver"),
    Kicker = task.GotoPos("Kicker", 150, r, 0),
    Goalie = task.Goalie()
},

["shoot_by_receiver"] = {
    switch = function()
        if CGetBallballx> 285 and CGetBallbally()< 70 and CGetBallbally()> -70 then
            return "deidai"
        end
    end,

    Receiver = task.Shoot("Receiver"),
    Kicker = task.GotoPos("Kicker", 150, r, 0),
    Goalie = task.Goalie()
},

-- =========================================================
-- 射门后判断：进了就结束，没进且球被扑出来就继续抢球补射
-- =========================================================
["deidai"] = {
    switch = function()
        if Cbuf_cnt(true, 80) then
            return "after_shoot_wait"
        end
    end,

    Kicker = task.Shoot("Kicker"),
    Receiver = task.GotoPos("Receiver", 150, r, 0),
    Goalie = task.Goalie()
},
["after_shoot_check"] = {
    switch = function()
        if isGoal() then
            return "finall1"
        elseif ballInField() and Cbuf_cnt(true, 30) then
            return "rebound_getball"
        elseif Cbuf_cnt(true, 180) then
            return "finall1"
        end
    end,

    Kicker = task.RobotHalt("Kicker"),
    Receiver = task.RobotHalt("Receiver"),
    Goalie = task.Goalie()
},

-- 判断谁离球近，谁去抢球
["rebound_getball"] = {
    switch = function()
        if kickerNearerBall() then
            return "kicker_rebound_getball"
        else
            return "receiver_rebound_getball"
        end
    end,

    Kicker = task.RobotHalt("Kicker"),
    Receiver = task.RobotHalt("Receiver"),
    Goalie = task.Goalie()
},

-- Kicker 抢球
["kicker_rebound_getball"] = {
    switch = function()
        if CBall2RoleDist("Kicker") < 12 then
            return "kicker_rebound_prepare_shoot"
        elseif Cbuf_cnt(true, 180) then
            return "after_shoot_check"
        end
    end,

    Kicker = task.GetBall("Kicker", "Kicker"),
    Receiver = task.GotoPos("Receiver", 150, r, 0),
    Goalie = task.Goalie()
},

-- Kicker 调整方向
["kicker_rebound_prepare_shoot"] = {
    switch = function()
        if Cbuf_cnt(true, 60) then
            return "kicker_rebound_shoot"
        end
    end,

    Kicker = task.GetBall("Kicker", "Kicker"),
    Receiver = task.GotoPos("Receiver", 150, r, 0),
    Goalie = task.Goalie()
},

-- Kicker 补射
["kicker_rebound_shoot"] = {
    switch = function()
        if Cbuf_cnt(true, 80) then
            return "after_shoot_check"
        end
    end,

    Kicker = task.Shoot("Kicker"),
    Receiver = task.GotoPos("Receiver", 150, r, 0),
    Goalie = task.Goalie()
},

-- Receiver 抢球
["receiver_rebound_getball"] = {
    switch = function()
        if CBall2RoleDist("Receiver") < 12 then
            return "receiver_rebound_prepare_shoot"
        elseif Cbuf_cnt(true, 180) then
            return "after_shoot_check"
        end
    end,

    Receiver = task.GetBall("Receiver", "Receiver"),
    Kicker = task.GotoPos("Kicker", 150, r, 0),
    Goalie = task.Goalie()
},

-- Receiver 调整方向
["receiver_rebound_prepare_shoot"] = {
    switch = function()
        if Cbuf_cnt(true, 60) then
            return "receiver_rebound_shoot"
        end
    end,

    Receiver = task.GetBall("Receiver", "Receiver"),
    Kicker = task.GotoPos("Kicker", 150, r, 0),
    Goalie = task.Goalie()
},

-- Receiver 补射
["receiver_rebound_shoot"] = {
    switch = function()
        if Cbuf_cnt(true, 80) then
            return "after_shoot_check"
        end
    end,

    Receiver = task.Shoot("Receiver"),
    Kicker = task.GotoPos("Kicker", 150, r, 0),
    Goalie = task.Goalie()
},

name = "test"
}