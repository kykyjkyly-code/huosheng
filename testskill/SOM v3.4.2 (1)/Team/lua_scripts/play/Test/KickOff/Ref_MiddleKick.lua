local Kickerdir2Receiverdir = function()
    return COurRole2RoleDir("Kicker", "Receiver")
end

gPlayTable.CreatePlay{

    firstState = "getball",

    ["getball"] = {
        switch = function()
            if CRole2TargetDist("Kicker") < 5 
               and CBall2RoleDist("Receiver") <13 then
                return "firstpass"
            end
        end,

        Receiver = task.GetBall("Receiver", "Kicker"),
        Kicker   = task.GotoPos("Kicker", 196, 30, Kickerdir2Receiverdir),
    },
    --牺牲时间让你调整

    ["firstpass"] = {
        switch = function()
            if Cbuf_cnt(true, 100) then
                return "dd"
            end
        end,

        Receiver = task.GetBall("Receiver", "Kicker"),
        Kicker   = task.GotoPos("Kicker", 196, 30, Kickerdir2Receiverdir),
    },

    ["dd"] = {
        switch = function()
            if CIsBallKick("Receiver") --or Cbuf_cnt(true, 120) then
                return "forshoot"
            end
        end,

        Kicker   = task.RobotHalt("Kicker"),
        Receiver = task.Passball("Receiver","Kicker"),
    },

    ["forshoot"] = {
        switch = function()
            if CBall2RoleDist("Kicker") < 10 
               --or Cbuf_cnt(true, 250) then
                return "shoot"
            end
        end,
--去一边，不要影响前锋射门
        Receiver = task.GotoPos("Receiver", -270, 0, 0),
    },

    ["shoot"] = {
        switch = function()
            if CGetBallX() > 285 
               and CGetBallY() < 70 
               and CGetBallY() > -70 then
                return "finall"
            end
        end,

        Kicker = task.Shoot("Kicker"),
    },

    

    name = "140"
}