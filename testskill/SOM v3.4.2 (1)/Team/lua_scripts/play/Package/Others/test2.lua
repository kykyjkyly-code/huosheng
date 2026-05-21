local ballx = function()
    return CGetBallX()
end

local dir2 = function()
    return COurRole2RoleDir("Kicker", "Receiver")
end

local dir3 = function()
    return COurRole2RoleDir("Receiver", "Kicker")
end
gPlayTable.CreatePlay{
	
firstState = "choose",

["choose"]={
    switch=function()
        if ballx() <-100 then
            return"back_get"
        else
            return"fm_get"
        end
    end,
    Receiver=task.RobotHalt("Receiver"),
    Kicker=task.RobotHalt("Kicker"),
},


-- 中前场拿球
["fm_get"]={
	switch=function()
        if CRole2TargetDist("Kicker") < 10 and CBall2RoleDist("Receiver")<10 or Cbuf_cnt(true,300) then
		   return"deng1"
        end
	end,

	Receiver=task.ReceiverTask("rao7"),
    Kicker=task.GotoPos("Kicker",120,120,dir2),
    
	
},
--等一会，让拿球稳定
 ["deng1"]={

    switch=function()
        if Cbuf_cnt(CBall2RoleDist("Receiver")<20,50) then
            return "pass"
        end
    end,
    Receiver=task.ReceiverTask("rao7"),
    Kicker=task.GotoPos(120,120,dir2),
},
--传球
["pass"]={
	switch=function()
	  if CIsBallKick("Receiver") and CBall2RoleDist("Receiver")  >10 then 
	   return "deng" 
	 end
	end, 

  Receiver=task.ReceiverTask("pass6"),
  Kicker=task.KickerTask("jia"),
}, 
--等一会让接的球稳定r
["deng"]={
    switch=function()
        if CBall2RoleDist("Kicker")<30 and CBall2RoleDist("Receiver")>20 and Cbuf_cnt(true,50) then
            return "sget"
        end
    end,
    Kicker=task.KickerTask("jia"),   
    Receiver=task.GotoPos("Receiver",-170,-100,0),
},

["sget"]={ 
    switch=function() 
        if Cbuf_cnt(true,200) then  
        return "shoot2"          
        end 
    end,
    Receiver=task.GotoPos("Receiver",-170,-100,0),
    Kicker=task.KickerTask("sget7"), 
}, 

["shoot2"]={
    switch=function()
      if CIsBallKick("Kicker") and CBall2RoleDist("Kicker")  >10 then 
       return "finish" 
     end
    end, 
    Receiver=task.GotoPos("Receiver",-170,-100,0),
    Kicker=task.KickerTask("shoot2"),
}, 




["back_get"]={
    switch=function()
        if CRole2TargetDist("Kicker") < 10 and CBall2RoleDist("Receiver")<10 or Cbuf_cnt(true,300) then
           return"bdeng"
        end
    end,

    Receiver=task.ReceiverTask("rao7"),
    Kicker=task.GotoPos("Kicker",0,120,0),
},

["bdeng"]={

    switch=function()
        if Cbuf_cnt(CBall2RoleDist("Receiver")<20,50) then
            return "bpass"
        end
    end,
    Receiver=task.ReceiverTask("rao7"),
    Kicker=task.KickerTask("jia"),
},
--传球
["bpass"]={
    switch=function()
        if CIsBallKick("Receiver") and CBall2RoleDist("Receiver")  >10 then 
            return "bdeng1" 
        end
    end, 

  Receiver=task.ReceiverTask("pass6"),
  Kicker=task.KickerTask("jia"),
}, 
--等一会让接的球稳定r

["bdeng1"]={
    switch=function()
        local ready =
            CBall2RoleDist("Kicker") < 10 and
            CRole2TargetDist("Receiver") < 10

        -- 建议让条件连续满足一段时间，不要一满足就跳
        if Cbuf_cnt(ready, 20) or Cbuf_cnt(true, 450) then
            return "bprepass2"
        end
    end,

    Kicker=task.KickerTask("rao7"),
    Receiver=task.GotoPos("Receiver",170,-120,dir3),
},

["bprepass2"]={
    switch=function()
        if Cbuf_cnt(true,300) then
            return "bpass2"
        end
    end,
    Kicker=task.KickerTask("rao7"),
    --Kicker=task.GotoPos("Kicker",0,120,dir2),
   -- Kicker=task.KickerTask("jia"),
    Receiver=task.GotoPos("Receiver",170,-120,dir3),
},

["bpass2"]={
    switch=function()
        if CIsBallKick("Kicker") and CBall2RoleDist("Kicker")  >10 then 
            return "bdeng2" 
        end
    end, 
    Kicker=task.KickerTask("pass6"),
    Receiver=task.ReceiverTask("jia"),
},

["bdeng2"]={
     switch=function()
        if CBall2RoleDist("Receiver")<30 and CBall2RoleDist("Kicker")>20 and Cbuf_cnt(true,50) then
            return "bsget"
        end
    end,
    Receiver=task.ReceiverTask("jia"),
    Kicker=task.RobotHalt("Kicker"),
},

["bsget"]={ 
    switch=function() 
        if Cbuf_cnt(true,200) then  
        return "bshoot2"          
        end 
    end,
    Receiver=task.ReceiverTask("sget7"),
    Kicker=task.RobotHalt("Kicker"),
  
}, 

["bshoot2"]={
    switch=function()
      if CIsBallKick("Receiver") and CBall2RoleDist("Receiver")  >10 then 
       return "finish" 
     end
    end, 

  
    Receiver=task.ReceiverTask("shoot2"),
    Kicker=task.RobotHalt("Kicker"),
}, 


name="test2"


}