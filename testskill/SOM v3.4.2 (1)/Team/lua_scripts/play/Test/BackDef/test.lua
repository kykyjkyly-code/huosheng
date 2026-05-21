--desc: 
gPlayTable.CreatePlay{
	
firstState = "start",

["start"]={
	switch=function()
		return"getball"
	end,

	Receiver=task.GetBall("Receiver","Kicker"),
	Kicker=task.GotoPos("Kicker",100.80,0),
},

["getball"]={
	switch=function()
	if CIsGetBall("Receiver") then 
	   return "passball" 
	 end
	end, 

    Receiver=task.GetBall("Receiver","Kicker"), 
    Kicker=task.GotoPos("Kicker",100,80,0), 
}, 

["passball"]={ 
    switch=function() 
        if CIsGetBall("Kicker") then
            return "shoot" 
        end 

        if CIsGetBall("Receiver") then
            return "getball" 
        end 
    end,

    Receiver=task.PassBall("Receiver","Kicker"), 
    Kicker=task.ReceiveBall("Kicker"), 
}, 

["shoot"]={ 
    switch=function() 
         if CIsBallKick("Kicker") then
       return "finish" 
   end
    end, 
   
    Receiver=task.Stop("Receiver",1), 
    Kicker=task.Shoot("Kicker"), 
}, 
    ["finish"] = {
        switch = function()
             if CIsBallKick("Kicker") then
             end

        end,

        Receiver = task.Stop("Receiver", 1),
        Kicker   = task.Stop("Kicker", 2),
    },



name="test"


}