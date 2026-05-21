local Kickerdir2Receiverdir = function()
    return COurRole2RoleDir("Kicker", "Receiver")
end
local Receiverdir2Kicker = function()
    return COurRole2RoleDir("Receiver", "Kicker")
end
gPlayTable.CreatePlay{

firstState = "start",

["start"] = {
    switch = function()
        if CNormalStart() then
          ---  return "K2"
        elseif CGameOn() then
            return "finish"
        end
    end,

    Kicker   = task.GotoPos(kicker,0,-5,Kickerdir2Receiverdir),
    Receiver = task.GotoPos(Receiver,0,40,Receiverdir2Kicker),
    Goalie   = task.Goalie()
},

["K2"] = {
    switch = function()
        if CBall2RoleDist("Kicker") > 25 then
            return "K3"
        end
    end,

    Kicker   = task.KickerTask("getball_1.0"),
    Receiver = task.ReceiverTask("POS_01"),
    Goalie   = task.Goalie()
},

["K3"] = {
    switch = function()
        if Cbuf_cnt(CBall2RoleDist("Receiver") < 15, 30) then
            return "K4"
        end
    end,

    Kicker   = task.KickerTask("POS_02"),
    Receiver = task.ReceiverTask("receiveball_1.0"),
    Goalie   = task.Goalie()
},

["K4"] = {
    switch = function()
        if Cbuf_cnt(CRole2TargetDist("Kicker") < 3, 50) then
            return "K5"
        end
    end,

    Kicker   = task.KickerTask("POS_02"),
    Receiver = task.ReceiverTask("gotopos"),
    Goalie   = task.Goalie()
},

["K5"] = {
    switch = function()
        if Cbuf_cnt(CBall2RoleDist("Receiver") > 20, 20) then
            return "K6"
        end
    end,

    Kicker   = task.KickerTask("POS_02"),
    Receiver = task.ReceiverTask("getball_1.0"),
    Goalie   = task.Goalie()
},

["K6"] = {
    switch = function()
        if Cbuf_cnt(CBall2RoleDist("Kicker") < 15, 30) then
            return "K7"
        end
    end,

    Kicker   = task.KickerTask("receiveball_1.0"),
    Receiver = task.ReceiverTask("POS_01"),
    Goalie   = task.Goalie()
},

["K7"] = {
    switch = function()
        if Cbuf_cnt(true, 50) then
            return "K8"
        end
    end,

    Kicker   = task.KickerTask("ready_shoot"),
    Receiver = task.ReceiverTask("POS_01"),
    Goalie   = task.Goalie()
},

["K8"] = {
    switch = function()
        if Cbuf_cnt(true, 30) then
            return "K8"
        end
    end,

    -- Receiver = task.ReceiverTask("shoot_left"),
    Kicker = task.KickerTask("shoot_left"),
},

name = "lwest"

}