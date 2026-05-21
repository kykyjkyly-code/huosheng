local Role2dir = function()
    return COurRole2RoleDir("Kicker", "Receiver")
end
local r = function()
    return 25
end
gPlayTable.CreatePlay{
	
firstState = "start",

["start"]={
	switch=function()
    --if CGetBallX()==220 and CGetBallY()==0 then
    --（220，0）是点球的坐标，要允许一点误差
        if CGetBallX()>200 and CGetBallX()<240
             and CGetBallY()>-50 and CGetBallY() < 50 then
            return"shoot"
        else
		    return"getball"
        end
	end,

	Receiver=task.RobotHalt("Receiver"),
	Kicker=task.RobotHalt("Kicker"),
},

["getball"]={
	switch=function()
        --准备好了再传 
        --if CBall2RoleDist("Receiver") < 30 then 
        if Cbuf_cnt(true,100) 
            and CIsGetBall("Receiver")
            and CBall2RoleDist("Receiver")<10
            and CRole2TargetDist("Kicker") <15
            then 
                return "passball" 
        end
        if Cbuf_cnt(true,300) then
                return "regetball"

	           
	    end
	end, 

    Receiver=task.GetBall("Receiver","Kicker"), 
    Kicker=task.GotoPos("Kicker",170,80,Role2dir), 
}, 
["regetball"] = {
    switch=function()
        if Cbuf_cnt(true,100)  then
            return "getball"
        end
    end,
           Receiver=task.GotoPos("Receiver",-70,80,Role2dir),

},

["passball"] = { 
    switch=function() 
        if CIsBallKick("Receiver") and CBall2RoleDist("Receiver") > 30  and CBall2RoleDist("Kicker") < 20   then
            return "beforeshoot"
        end
    end,

    Receiver=task.ReceiverTask("passballmy"), 
   -- Kicker=task.GotoPos("Kicker",170,80,Role2dir), 
}, 

["beforeshoot"]={
    switch=function()
        if  Cbuf_cnt(true,100) then
            return "shoot"
        end
    end,

    Receiver=task.RobotHalt("Receiver"),
    Kicker=task.GetBall("Kicker","Kicker"),
},

["shoot"]={ 
    switch=function()
        if CGetBallX() > 285  and CGetBallY() < 70 and CGetBallY() > -70 then
            return "wait" 
        end
    end, 
   
    Receiver=task.RobotHalt("Receiver"), 
    Kicker=task.Shoot("Kicker"), 
    
}, 

["wait"] = {
    switch = function()
        if Cbuf_cnt(true,50) then
            return "kicker_rebound_getball"
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
            return "wait"
        end
    end,

    Kicker = task.Shoot("Kicker"),
    Receiver = task.GotoPos("Receiver", 150, r, 0),
    Goalie = task.Goalie()
},



--["aftershoot"]={
    --switch=function()
        --if CRole2TargetDist("Kicker") < 5 then
            --return "finish"
        --end
    --end,

    --Receiver=task.RobotHalt("Receiver"),
    --Kicker=task.KickerTask("myaftershoot"),
--},
    
--["finish"]={ 
   --switch=function() 
   --end, 
    
   --Receiver=task.RobotHalt("Receiver"), 
   --Kicker=task.RobotHalt("Kicker"), 
--}, 

name="testlw"


}