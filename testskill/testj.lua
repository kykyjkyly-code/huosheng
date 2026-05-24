--==================== 功能块 1：基础取值函数 ====================
-- ballx：获取当前球的 X 坐标，用来判断球在前场还是后场。
-- dir2：Kicker 朝向 Receiver 的方向。
-- dir3：Receiver 朝向 Kicker 的方向。

local ballx = function()
    return CGetBallX()
end

local dir2 = function()
    return COurRole2RoleDir("Kicker", "Receiver")
end

local dir3 = function()
    return COurRole2RoleDir("Receiver", "Kicker")
end


--==================== 功能块 2：创建战术脚本 ====================
-- firstState 表示战术从 choose 状态开始执行。
gPlayTable.CreatePlay{
    
firstState = "choose",


--==================== 状态 1：选择前场或后场流程 ====================
-- 根据球的 X 坐标判断当前属于后场拿球还是中前场拿球。
-- ballx() < -100：进入后场流程 back_get
-- 否则：进入中前场流程 fm_get
["choose"]={
    switch=function()
        if ballx() <-100 then
           -- return"back_get"
        else
            return"fm_get"
        end
    end,
    Receiver=task.RobotHalt("Receiver"),
    Kicker=task.RobotHalt("Kicker"),
},


--==========================================================
-- 功能块 3：中前场进攻流程
-- 流程：
-- fm_get → deng1 → pass → deng → sget → 1shoot → finish
--==========================================================


--==================== 状态 2：中前场拿球准备 ====================
-- Receiver 执行 3jie 拿球任务。
-- Kicker 跑到指定接应点。
-- 当 Kicker 到点，并且 Receiver 接近球，或者超时，就进入等待状态 deng1。
["fm_get"]={
    switch=function()
        if CRole2TargetDist("Kicker") < 20 and CBall2RoleDist("Receiver")<20 or Cbuf_cnt(true,300) then
           return"deng1"
        end
    end,

    Receiver=task.ReceiverTask("pget10"),
    Kicker=task.KickerTask("3jie"),
    
    
},


--==================== 状态 3：中前场拿球稳定等待 ====================
-- Receiver 继续执行 3jie。
-- 等 Receiver 和球距离持续满足条件后，进入传球状态 pass。
["deng1"]={

    switch=function()
        if Cbuf_cnt(CBall2RoleDist("Receiver")<25,145) then
            return "pass"
        end
    end,
    Receiver=task.ReceiverTask("accpget7"),
    Kicker=task.KickerTask("3jie"),
},


--==================== 状态 4：Receiver 传球给 Kicker ====================
-- Receiver 执行 7pass 传球任务。
-- Kicker 执行 3jie 接球/辅助任务。
-- 检测到 Receiver 已踢球，并且球离开 Receiver 后，进入 deng。
["pass"]={
    switch=function()
      if CIsBallKick("Receiver") and CBall2RoleDist("Receiver")  >20 then 
       return "deng" 
     end
    end, 

  Receiver=task.ReceiverTask("pass0.15"),
  Kicker=task.KickerTask("3jie"),
}, 


--==================== 状态 5：Kicker 接球稳定等待 ====================
-- Kicker 继续执行 3jie。
-- Receiver 跑到指定位置。
-- 当 Kicker 接近球、Receiver 离球较远，并等待一段时间后，进入 sget。
["deng"]={
    switch=function()
        if CBall2RoleDist("Kicker")<30 and CBall2RoleDist("Receiver")>20 or Cbuf_cnt(true,150) then
            return "sget"
        end
    end,
    Kicker=task.KickerTask("3jie"),   
    Receiver=task.GotoPos("Receiver",-170,-100,0),
},


--==================== 状态 6：Kicker 射门前拿球调整 ====================
-- Kicker 执行 3sget，准备控球或调整射门姿态。
-- 等待一段时间后进入 1shoot。
["sget"]={ 
    switch=function() 
        if Cbuf_cnt(CBall2RoleDist("Kicker")<50,30) then  
        return "1shoot"          
        end 
    end,
       --Receiver=task.RobotHalt("Receiver"),
    Receiver=task.GotoPos("Receiver",-170,-100,0),
    Kicker=task.KickerTask("3sget"), 
}, 


--==================== 状态 7：Kicker 射门 ====================
-- Kicker 执行 1shoot 射门任务。
-- 检测到 Kicker 已踢球，并且球离开 Kicker 后，进入 finish。
["1shoot"]={
    switch=function()
       if CBall2RoleDist("Kicker")>30 then 
       return "finish" 
       end
    end, 
       Receiver=task.RobotHalt("Receiver"),
    --Receiver=task.GotoPos("Receiver",-170,-100,0),
    Kicker=task.KickerTask("2shootping"),
}, 


-- 检测到 Kicker 已踢球且离开球后，进入 finish
--["finish"] = {
   -- switch = function()
        -- 可以加判断条件，如果需要判断球或距离
        --return nil -- finish 状态保持
    --end,
   -- Receiver = task.RobotHalt("Receiver"),   -- 立即停止 Receiver
   -- Kicker   = task.RobotHalt("Kicker"),     -- 立即停止 Kicker
--},



--==========================================================
-- 功能块 4：后场进攻流程
-- 流程：
-- back_get → bdeng → bpass → bdeng1 → bpass2
--      → bdeng2 → bsget → b1shoot → finish
--==========================================================


--==================== 状态 8：后场拿球准备 ====================
-- Receiver 执行 4jie 拿球任务。
-- Kicker 跑到后场接应点。
-- 当 Kicker 到点，并且 Receiver 接近球，或者超时，就进入 bdeng。
["back_get"]={
    switch=function()
        if CRole2TargetDist("Kicker") < 25 and CBall2RoleDist("Receiver")<25 then
           return"bdeng"
        end
    end,

    Receiver=task.ReceiverTask("pget10"),
    Kicker=task.KickerTask("3jie"),
},


--==================== 状态 9：后场第一次拿球稳定等待 ====================
-- Receiver 继续拿球。
-- Kicker 执行 3jie。
-- 等 Receiver 靠近球一段时间，或等待超时后，进入 bpass。
["bdeng"]={

    switch=function()
        if Cbuf_cnt(CBall2RoleDist("Receiver")<25,150) or Cbuf_cnt(true,200) then
            return "bpass"
        end
    end,
    Receiver=task.ReceiverTask("accpget7"),
    Kicker=task.KickerTask("3jie"),
},


--==================== 状态 10：后场第一次传球 ====================
-- Receiver 执行 7pass，把球传给 Kicker。
-- Kicker 执行 4jie 接球。
-- 检测到 Receiver 已踢球，并且球离开 Receiver 后，进入 bdeng1。
["bpass"]={
    switch=function()
        if CIsBallKick("Receiver") and CBall2RoleDist("Receiver")  >20 then 
            return "bdeng1" 
        end
    end, 

    Receiver=task.ReceiverTask("pass0.15"),
    Kicker=task.KickerTask("3jie"),
}, 


--==================== 状态 11：后场第二次传球前准备 ====================
-- Kicker 执行 4jie，准备拿球或调整。
-- Receiver 跑到第二次接球点。
-- 条件稳定满足或超时后，进入 bpass2。
["bdeng1"]={
    switch=function()
        local ready =
            CBall2RoleDist("Kicker") < 30 and
            CRole2TargetDist("Receiver") < 30

        -- 建议让条件连续满足一段时间，不要一满足就跳
        if Cbuf_cnt(ready, 20) or Cbuf_cnt(true, 10) then
            return "bprepass2"
           --return "bpass2"
        end
    end,

    Kicker=task.KickerTask("3jie"),
    Receiver=task.ReceiverTask("3jie2ci"),

},


--==================== 预留状态：第二次传球前缓冲 ====================
-- 当前整段被注释掉，没有参与实际流程。
-- 原本作用可能是让 Kicker 先调整位置，再进入 bpass2。
["bprepass2"]={
    switch=function()
         if Cbuf_cnt(CBall2RoleDist("Kicker")<30,100) or Cbuf_cnt(true,200) then
            return "bpass2"
        end
    end,
    Kicker=task.KickerTask("accpget7"),
    --Kicker=task.GotoPos("Kicker",0,120,dir2),
    --Kicker=task.KickerTask("4jie"),
    --Receiver=task.GotoPos("Receiver",120,-100,dir3),
    Receiver=task.ReceiverTask("3jie2ci")
},


--==================== 状态 12：后场第二次传球 ====================
-- Kicker 执行 7pass，把球传给 Receiver。
-- Receiver 执行 4jie 接球。
-- 检测到 Kicker 已踢球，并且球离开 Kicker 后，进入 bdeng2。
["bpass2"]={
    switch=function()
        if CIsBallKick("Kicker") and CBall2RoleDist("Kicker")  >20 then 
            return "bdeng2" 
        end
    end, 
    Kicker=task.KickerTask("pass0.15"),
    Receiver=task.ReceiverTask("3jie"),
},


--==================== 状态 13：Receiver 接球稳定等待 ====================
-- Receiver 继续执行 4jie。
-- Kicker 停止。
-- 当 Receiver 接近球、Kicker 离球较远，并等待一段时间后，进入 bsget。
["bdeng2"]={
     switch=function()
        if CBall2RoleDist("Receiver")<30 and CBall2RoleDist("Kicker")>20 or Cbuf_cnt(true,150) then
            return "bsget"
        end
    end,
    Kicker=task.RobotHalt("Kicker"),
    Receiver=task.ReceiverTask("3jie2ci"),
    
},


--==================== 状态 14：Receiver 射门前拿球调整 ====================
-- Receiver 执行 3sget，准备控球或调整射门姿态。
-- Kicker 停止。
-- 等待一段时间后进入 b1shoot。
["bsget"]={ 
    switch=function() 
        if Cbuf_cnt(true,100) then  
        return "b1shoot"          
        end 
    end,
    Kicker=task.RobotHalt("Kicker"),
    Receiver=task.ReceiverTask("3sget"),
    
  
}, 


--==================== 状态 15：Receiver 射门 ====================
-- Receiver 执行 1shoot 射门任务。
-- 检测到 Receiver 已踢球，并且球离开 Receiver 后，进入 finish。
["b1shoot"]={
    switch=function()
      if CIsBallKick("Receiver") and CBall2RoleDist("Receiver")  >10 then 
       return "finish" 
     end
    end, 
    Kicker=task.RobotHalt("Kicker"),
    Receiver=task.ReceiverTask("2shootping"),
    
}, 


--==================== 功能块 5：战术名称 ====================
-- name 是该 play 的名称，用于脚本管理和调用。
name="testj"


}