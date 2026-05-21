--desc: 
gPlayTable.CreatePlay{
	
firstState = "getball",
-- 拿球
["getball"]={
	switch=function()
        if CRole2TargetDist("Kicker") < 10 and CBall2RoleDist("Receiver")<10 or Cbuf_cnt(true,300) then
		   return"deng1"
        end
	end,

	Receiver=task.ReceiverTask("rao7"),
   --- Kicker=task.GotoPos("Kicker",270,0,0),
    Kicker=task.KickerTask("jie"),
	
},
--等一会，让拿球稳定
["deng1"]={

    switch=function()
        if Cbuf_cnt(CBall2RoleDist("Receiver")<20,50) then
            return "pass"
        end
    end,
  Receiver=task.ReceiverTask("rao7"),
    Kicker=task.KickerTask("jie"),
},
--传球
["pass"]={
	switch=function()
	  if CIsBallKick("Receiver") and CBall2RoleDist("Receiver")  >10 then 
	   return "deng" 
	 end
	end, 

  Receiver=task.ReceiverTask("pass6"),
  Kicker=task.KickerTask("jie"),
}, 
--等一会让接的球稳定r
["deng"]={

    switch=function()
        if CBall2RoleDist("Kicker")<30 and CBall2RoleDist("Receiver")>20 and Cbuf_cnt(true,50) then
            return "sget"
        end
    end,
    Kicker=task.KickerTask("jie"),   
    Receiver=task.GotoPos("Receiver",-270,0,0),
},

["sget"]={ 
    switch=function() 
        if Cbuf_cnt(true,200) then  
        return "shoot"          
        end 
    end,
    
    
    Receiver=task.GotoPos("Receiver",-270,0,0),
    Kicker=task.KickerTask("sget7"),
  
}, 

["shoot"]={
    switch=function()
      if CIsBallKick("Kicker") and CBall2RoleDist("Kicker")  >10 then 
       return "deng23" 
     end
    end, 

  
    Receiver=task.GotoPos("Receiver",-270,0,0),
  Kicker=task.KickerTask("shoot"),
}, 

name="test1"


}