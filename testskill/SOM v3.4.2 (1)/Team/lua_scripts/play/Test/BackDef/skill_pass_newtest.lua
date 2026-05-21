--desc: 
gPlayTable.CreatePlay{

firstState = "GetBall",

["GetBall"] = {
    switch = function()
        -- 当Receiver拿到球
        if CIsGetBall("Receiver")  then
            return "Pass"
        end
    end,

    Kicker = task.GoRecePos("Kicker"),
    Receiver = task.GetBall("Receiver","Receiver"),
    Goalie = task.Goalie(),
},

["Pass"] = {
    switch = function()
        if CIsGetBall("Kicker") then
            return finish

        end
    end,
    
    Kicker = task.KickerTask("myskill"),
    Receiver  = task.ReceiverTask("skill_pass"),
    Goalie = task.Goalie(),

},






name="skill_pass_newtest"
}