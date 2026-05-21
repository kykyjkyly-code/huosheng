#ifndef GETBALLSOURCE_H
#define GETBALLSOURCE_H

#include "utils\PlayerTask.h"

class WorldModel;

class GetBall
{
public:
	GetBall();
	~GetBall();

	PlayerTask plan(const WorldModel* model, int robot_id, int receiver_id);
	bool toward_opp_goal(float dir);
	float ball_x_angle(const WorldModel* model);
};

typedef Singleton<GetBall> getBall;

#endif
