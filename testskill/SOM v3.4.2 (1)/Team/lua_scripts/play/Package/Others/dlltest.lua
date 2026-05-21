local rdaok = function()
    return COurRole2RoleDir("Receiver", "Kicker")
end
local kdaor = function()
    return COurRole2RoleDir("Kicker","Receiver")
end
local r = function()
    return 25
end
local x1 = function()
    return 66
end
local y1 = function()
    return -80
end
local x2 = function()
    return 192
end
local y2 = function()
    return 102
end
gPlayTable.CreatePlay{
	
firstState = "start",

["start"]={
	switch=function()
    --if CGetBallX()==220 and CGetBallY()==0 then
    --（220，0）是点球的坐标要，允许一点误差
        if CGetBallX()>200 and CGetBallX()<240
             and CGetBallY()>-50 and CGetBallY() < 50 then
            return"shoot"
        else
		    return"getball"x
        end
	end,

	Kicker=task.RobotHalt("Kicker"),
	Receiver=task.RobotHalt("Receiver"),
    Goalie = task.Goalie(),
},
---1
["getball"]={
	switch=function()
        --准备好了再传 
        --if CBall2RoleDist("Kicker") < 30 then 
        if Cbuf_cnt(true,100) 
            and CIsGetBall("Kicker")
            and CBall2RoleDist("Kicker")<10
            and CRole2TargetDist("Receiver") <15
            or Cbuf_cnt(true,150)
            then return "regetball"
        
    	           
	    end
	end, 

    Kicker=task.GetBall("Kicker","Receiver"), 
    Receiver=task.GotoPos("Receiver",x1,y1,rdaok), 
     Goalie = task.Goalie(),
}, 
---2
["regetball"] = {
    switch=function()
        if Cbuf_cnt(true,100)  then
            return "passball"
        end
    end,
           
    Kicker=task.GetBall("Kicker","Receiver"), 
    Receiver=task.GotoPos("Receiver",x1,y1,rdaok), 
     Goalie = task.Goalie(),
},
---传球
["passball"] = { 
    switch=function() 
        if CIsBallKick("Kicker") and CBall2RoleDist("Kicker") > 30  and CBall2RoleDist("Receiver") < 20   then
            return "beforeshoot"
        end
    end,

    Kicker=task.KickerTask("passballtwice"), 
     Goalie = task.Goalie(),
   -- Receiver=task.GotoPos("Receiver",170,80,rdaok), 
}, 
---跑位，去拿球
["beforeshoot"]={
    switch=function()
        if  Cbuf_cnt(true,100) and CRole2TargetDist("Receiver") <15 then
            return "twogetball"
        end
    end,

    Kicker=task.GotoPos("Kicker",x2,y2,kdaor),
    Receiver=task.GetBall("Receiver","Kicker"),
     Goalie = task.Goalie(),
},
---这个是什么
----1
["twogetball"] = {
    switch=function()
        if Cbuf_cnt(true,100)  then
            return "passball1"
        end
    end,
           
   Kicker=task.GotoPos("Kicker",x2,y2,kdaor),
    Receiver=task.GetBall("Receiver","Kicker"),
     Goalie = task.Goalie(),
},
---第二次传球
---2

["passball1"] = { 
    switch=function() 
        if CIsBallKick("Receiver") and CBall2RoleDist("Receiver") > 30  and CBall2RoleDist("Kicker") < 20   then
            return "beforeshoot1"
        end
    end,

    Receiver=task.ReceiverTask("passballtwice"), 
     Goalie = task.Goalie(),
   --- Kicker=task.GetBall("Kicker","Receiver"), 
}, 

["beforeshoot1"]={
    switch=function()
        if  Cbuf_cnt(true,100) and CRole2TargetDist("Kicker")<15 then
            return "regetball1"
        end
    end,

    Kicker=task.GetBall("Kicker","Kicker"),
    Receiver=task.RobotHalt("Receiver"),
     Goalie = task.Goalie(),
},

["regetball1"] = {
    switch=function()
        if Cbuf_cnt(true,100)  then
            return "shoot"
        end
    end,
           
   Kicker=task.GetBall("Kicker","Kicker"),
    Receiver=task.RobotHalt("Receiver"),
     Goalie = task.Goalie(),
},

["shoot"]={ 
    switch=function()
        if CGetBallX() > 285  and CGetBallY() < 70 and CGetBallY() > -70 then
            return "wait" 
        end
    end, 
   
    Receiver=task.RobotHalt("Receiver"), 
    Kicker=task.Shoot("Kicker"),
     Goalie = task.Goalie(), 
    
}, 

["wait"] = {
    switch = function()
        if Cbuf_cnt(true,50) then
            return "Receiver_rebound_getball"
        end
    end,
    Receiver = task.RobotHalt("Receiver"),
    Kicker = task.RobotHalt("Kicker"),
    Goalie = task.Goalie()
},


-- Receiver 抢球
["Receiver_rebound_getball"] = {
    switch = function()
        if CBall2RoleDist("Receiver") < 12 then
            return "Receiver_rebound_prepare_shoot"
        
        end
    end,

    Receiver = task.GetBall("Receiver", "Receiver"),
    Kicker = task.GotoPos("Kicker", 150, r, 0),
    Goalie = task.Goalie()
},

-- Receiver 调整方向
["Receiver_rebound_prepare_shoot"] = {
    switch = function()
        if Cbuf_cnt(true, 60) then
            return "Receiver_rebound_shoot"
        end
    end,

    Receiver = task.GetBall("Receiver", "Receiver"),
    Kicker = task.GotoPos("Kicker", 150, r, 0),
    Goalie = task.Goalie()
},

-- Receiver 补射
["Receiver_rebound_shoot"] = {
    switch = function()
        if Cbuf_cnt(true, 80) then
            return "wait"
        end
    end,

    Receiver = task.Shoot("Receiver"),
    Kicker = task.GotoPos("Kicker", 150, r, 0),
    Goalie = task.Goalie()
},



--["aftershoot"]={
    --switch=function()
        --if CRole2TargetDist("Receiver") < 5 then
            --return "finish"
        --end
    --end,

    --Kicker=task.RobotHalt("Kicker"),
    --Receiver=task.ReceiverTask("myaftershoot"),
--},
    
--["finish"]={ 
   --switch=function() 
   --end, 
    
   --Kicker=task.RobotHalt("Kicker"), 
   --Receiver=task.RobotHalt("Receiver"), 
--}, 

name="dlltest"


}