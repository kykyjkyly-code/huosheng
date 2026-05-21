--desc: 
--改进的话就是从
--
local ballx = function()
    return CGetBallX()
end
local bally = function()
    return CGetBallY()
end
local ballfordoor = function()
    return CBall2PointDir(300,0)
end
local Kickerdir2Receiverdir = function()
    return COurRole2RoleDir("Kicker", "Receiver")
end
local rdk = function()
    return COurRole2RoleDir("Receiver", "Kicker")
end
gPlayTable.CreatePlay{

firstState = "getball",
--判断是否是点球的情况
["start"] = {
    switch = function ()
        if CNormalStart() then
            ---return "if"
        elseif CGameOn() then
            return "finish"
        end
    end,
    Kicker = task.GotoPos("Kicker",-50,0,Kickerdir2Receiverdir),
    Receiver = task.GotoPos("Receiver",0,50,rdk),
   Goalie = task.Goalie()
},
["if"] = {
       switch = function()
            if CGetBallX()>200 and CGetBallY()<100 and CGetBallY()>-100 then
                return "shoot"
            else
                return "getball"
            end
        end,
},

--拿球跑位
--前锋去目标点，中场去拿球
--有一个重要的问题，我forpass改为pass之后，把球喂到前锋嘴里，会射门，中场还是会传球，前锋也能接球，有一些东西是自动的，暂时没摸清原理
["getball"] = {
        switch = function()
            if CRole2TargetDist("Kicker") < 5 and CBall2RoleDist("Receiver")<10 or Cbuf_cnt(true,300) then
                
              
               --- return "forpass"
            end
        end,

        Receiver = task.ReceiverTask("testgetball"),
        Kicker   = task.GotoPos("Kicker", 156,20,Kickerdir2Receiverdir),
        Goalie = task.Goalie()
},
   
--等一会，方向稳定
["forpass"] = {
       switch = function()
            if Cbuf_cnt(true,100) then
              return "pass"
            end
       end,

       Receiver = task.GetBall("Receiver", "Kicker"),
        Kicker   = task.GotoPos("Kicker", 184, -95,Kickerdir2Receiverdir),
        Goalie = task.Goalie()
},
--开始传
--判断传出，并且成功，延时防止
["pass"] = {
        switch = function()
            if CIsBallKick("Receiver") and CBall2RoleDist("Receiver")  >10  or Cbuf_cnt(true, 120) then
           
                return "forshoot"
            end
        end,

        --Kicker   = task.RobotHalt("Kicker"),
        Receiver = task.ReceiverTask("mypassball"),
        Goalie = task.Goalie()
},
--中场别碍事,
["forshoot"] = {
        switch = function()
            if CBall2RoleDist("Kicker") < 20   then
              
                return "Kickerforshoot"
           -- elseif Cbuf_cnt(true,250) then
             --   return "Kickerforshoot"
            end
        end,
      --Kicker   = task.RobotHalt("Kicker"),
        Receiver= task.GotoPos("Receiver", -270,0,0),
        Goalie = task.Goalie()
},
--调整前锋的方向
["Kickerforshoot"] = {
    switch =function()
        if Cbuf_cnt(true,100) then
            return "shoot"
        end
    end,
              Kicker=task.GetBall("Kicker","Kicker"),
            Goalie = task.Goalie()
        
},
["shoot"] = {
        switch = function()
            if CGetBallX() > 285  and CGetBallY() < 70 and CGetBallY() > -70 then
            
                return "finall"
            end
        end,
        --Receiver = task.GotoPos("Receiver", -270, 0, 0),
        Kicker = task.Shoot("Kicker"),
        Goalie = task.Goalie()
},

    

name = "testpro11"
}