#if 0
#include <cmath>
#include <cstring>
#include <ostream>
#include "src\utils\PlayerTask.h"
#pragma warning(disable: 4190)

#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"

/*
Official PassBall converted to the testskill DLL entry style.
Enable this file with #if 1, and keep every other cpp that exports
player_plan disabled with #if 0.
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== official pass params ====================*/
const int OFFICIAL_PASS_FIXED_RECEIVER_ID = -1;  /* -1 means auto choose. */
const float OFFICIAL_PASS_RECEIVER_HEAD_LENGTH = 7.0f;
const float OFFICIAL_PASS_FAST_DIST = 3.0f;
const float OFFICIAL_PASS_READY_ANGLE = 0.5f;
const float OFFICIAL_PASS_GET_BALL_DIST_OFFSET = -1.5f;
const float OFFICIAL_PASS_MOUTH_ANGLE = static_cast<float>(PI / 6);
const float OFFICIAL_PASS_NOT_READY_BACK_DIST = static_cast<float>(BALL_SIZE / 2 + MAX_ROBOT_SIZE + 12);
const double OFFICIAL_PASS_KICK_POWER = 127.0;

bool officialPassValidRobot(int robot_id)
{
	return robot_id >= 0 && robot_id < 6;
}

int officialPassFindReceiver(const WorldModel* model, int robot_id)
{
	const bool* our_exist = model->get_our_exist_id();
	const int goalie_id = model->get_our_goalie();

	if (our_exist == NULL)
	{
		return -1;
	}

	if (OFFICIAL_PASS_FIXED_RECEIVER_ID >= 0 && OFFICIAL_PASS_FIXED_RECEIVER_ID < 6 &&
		OFFICIAL_PASS_FIXED_RECEIVER_ID != robot_id &&
		OFFICIAL_PASS_FIXED_RECEIVER_ID != goalie_id &&
		our_exist[OFFICIAL_PASS_FIXED_RECEIVER_ID])
	{
		return OFFICIAL_PASS_FIXED_RECEIVER_ID;
	}

	for (int i = 0; i < 6; i++)
	{
		if (i == robot_id || i == goalie_id)
		{
			continue;
		}

		if (our_exist[i])
		{
			return i;
		}
	}

	return -1;
}

bool officialPassReady(const point2f& ball_pos, const point2f& passer_pos, const point2f& receiver_pos)
{
	const float receiver_to_ball_dir = (ball_pos - receiver_pos).angle();
	const float ball_to_passer_dir = (passer_pos - ball_pos).angle();
	return fabs(Maths::normalizeAngle(receiver_to_ball_dir - ball_to_passer_dir)) < OFFICIAL_PASS_READY_ANGLE;
}

bool officialPassHasBall(const WorldModel* model, int robot_id)
{
	const point2f& passer_pos = model->get_our_player_pos(robot_id);
	const point2f& ball_pos = model->get_ball_pos();
	const float passer_dir = model->get_our_player_dir(robot_id);
	const point2f passer_to_ball = ball_pos - passer_pos;

	return passer_to_ball.length() < get_ball_threshold + OFFICIAL_PASS_GET_BALL_DIST_OFFSET &&
		fabs(Maths::normalizeAngle(passer_dir - passer_to_ball.angle())) < OFFICIAL_PASS_MOUTH_ANGLE;
}

PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	PlayerTask task;

	if (model == NULL || !officialPassValidRobot(robot_id))
	{
		return task;
	}

	int receiver_id = officialPassFindReceiver(model, robot_id);
	if (receiver_id == -1)
	{
		receiver_id = robot_id;
	}

	const float receiver_dir = model->get_our_player_dir(receiver_id);
	const point2f& receiver_pos = model->get_our_player_pos(receiver_id);
	const point2f& passer_pos = model->get_our_player_pos(robot_id);
	const point2f& ball_pos = model->get_ball_pos();
	const point2f opponent_goal = -FieldPoint::Goal_Center_Point;

	const point2f receiver_head_pos = receiver_pos + Maths::vector2polar(OFFICIAL_PASS_RECEIVER_HEAD_LENGTH, receiver_dir);
	const float receiver_to_ball_dir = (ball_pos - receiver_pos).angle();
	const float pass_dir = (receiver_head_pos - ball_pos).angle();
	const bool has_ball = officialPassHasBall(model, robot_id);

	task.needCb = true;
	task.isPass = true;
	task.isChipKick = false;

	if (receiver_id == robot_id)
	{
		if (has_ball)
		{
			task.kickPower = OFFICIAL_PASS_KICK_POWER;
			task.needKick = true;
		}

		task.target_pos = ball_pos + Maths::vector2polar(OFFICIAL_PASS_FAST_DIST, (ball_pos - opponent_goal).angle());
		task.orientate = (opponent_goal - ball_pos).angle();
		task.flag = 1;
		return task;
	}

	if (officialPassReady(ball_pos, passer_pos, receiver_pos))
	{
		if (has_ball)
		{
			task.kickPower = OFFICIAL_PASS_KICK_POWER;
			task.needKick = true;
		}

		task.target_pos = ball_pos + Maths::vector2polar(OFFICIAL_PASS_FAST_DIST, receiver_to_ball_dir);
	}
	else
	{
		task.target_pos = ball_pos + Maths::vector2polar(OFFICIAL_PASS_NOT_READY_BACK_DIST, receiver_to_ball_dir);
	}

	task.orientate = pass_dir;
	task.flag = 1;
	return task;
}
#endif
