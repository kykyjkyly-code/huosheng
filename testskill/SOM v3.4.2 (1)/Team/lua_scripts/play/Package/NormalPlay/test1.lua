gPlayTable.CreatePlay{
	
firstState = "getball",
-- 拿球
["getball"]={
	switch=function()
        if CRole2TargetDist("Kicker") < 5 and CBall2RoleDist("Receiver")<10 or Cbuf_cnt(true,300) then
		    return"deng1"
        end
	end,

	Receiver=task.ReceiverTask("pget"),
    Kicker=task.KickerTask("jie"),
	
},
--等一会，让拿球稳定
["deng1"]={

    switch=function()
        if Cbuf_cnt(true,50) then
            return "pass"
        end
    end,
  Receiver=task.ReceiverTask("pget"),
    Kicker=task.KickerTask("jie"),
},
--传球
["pass"]={
	switch=function()
	 if CBall2RoleDist("Kicker") < 10  then 
	   return "deng" 
	 end
	end, 

  Receiver=task.ReceiverTask("pass"),
  Kicker=task.KickerTask("jie"),
}, 
--等一会让接的球稳定
["deng"]={

    switch=function()
        if Cbuf_cnt(true,50) then
            return "sget"
        end
    end,
    Kicker=task.KickerTask("jie"),
    Receiver=task.RobotHalt("Receiver"),
},

["sget"]={ 
    switch=function() 
        if CBall2RoleDist("Kicker") < 30 then 
        ---    return "shoot" 
        end 

        if CBall2RoleDist("Receiver")< 30 then 
           --- return "getball" 
        end 
    end,
    
    
    Receiver=task.GotoPos("Receiver",-270,0,0),
    Kicker=task.KickerTask("sget6"),
  
}, 


name="test1"


}