gPlayTable.CreatePlay{

firstState = "fm_get",

["fm_get"]={
    switch=function()
        if CRole2TargetDist("Kicker") < 20 and CBall2RoleDist("Receiver")<20 or Cbuf_cnt(true,300) then
           return"deng1"
        end
    end,

    Receiver=task.ReceiverTask("pget10"),
    Kicker=task.KickerTask("3jie"),
    Goalie=task.GoalieTask("smartgoalie"),
    
    
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
    Goalie=task.GoalieTask("smartgoalie"),
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
  Goalie=task.GoalieTask("smartgoalie"),
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
    Goalie=task.GoalieTask("smartgoalie"),
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
    Goalie=task.GoalieTask("smartgoalie"),
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
    Goalie=task.GoalieTask("smartgoalie"),
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

name="middlestart"

}