local rdaok = function()
    return COurRole2RoleDir("Receiver", "Kicker")
end
local kdaor = function()
    return COurRole2RoleDir("Kicker","Receiver")
end
local r = function()
    return 25
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
		    return"getball"
        end
	end,

	Kicker=task.RobotHalt("Kicker"),
	Receiver=task.RobotHalt("Receiver"),
},

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
    Receiver=task.GotoPos("Receiver",66,-126,rdaok), 
}, 

["regetball"] = {
    switch=function()
        if Cbuf_cnt(true,100)  then
            return "passball"
        end
    end,
           
    Kicker=task.GetBall("Kicker","Receiver"), 
    Receiver=task.GotoPos("Receiver",66,-126,rdaok), 
},

["passball"] = { 
    switch=function() 
        if CIsBallKick("Kicker") and CBall2RoleDist("Kicker") > 30  and CBall2RoleDist("Receiver") < 20   then
            return "beforeshoot"
        end
    end,

    Kicker=task.KickerTask("passballtwice"), 
   -- Receiver=task.GotoPos("Receiver",170,80,rdaok), 
}, 

["beforeshoot"]={
    switch=function()
        if  Cbuf_cnt(true,100) and CRole2TargetDist("Receiver") <15 then
            return "passball1"
        end
    end,

    Kicker=task.GotoPos("Kicker",191,102,kdaor),
    Receiver=task.GetBall("Receiver","Kicker"),
},
---如果rece拿不好球的话，在这里再加一个等待函数
["passball1"] = { 
    switch=function() 
        if CIsBallKick("Receiver") and CBall2RoleDist("Receiver") > 30  and CBall2RoleDist("Kicker") < 20   then
            return "beforeshoot1"
        end
    end,

    Receiver=task.ReceiverTask("passballtwice"), 
    Kicker=task.GotoPos("Kicker",191,102,rdaok), 
}, 

["beforeshoot1"]={
    switch=function()
        if  Cbuf_cnt(true,100)  then
            return "shoot"
        end
    end,

    Kicker=task.GotoPos("Kicker",191,102,kdaor),
    Receiver=task.GetBall("Receiver","Kicker"),
},

["shoot"]={ 
    switch=function()
        if CGetBallX() > 285  and CGetBallY() < 70 and CGetBallY() > -70 then
            return "wait" 
        end
    end, 
   
    Kicker=task.RobotHalt("Kicker"), 
    Receiver=task.Shoot("Receiver"), 
    
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