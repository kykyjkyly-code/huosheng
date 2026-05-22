#if 0
#include <cmath>
#include <cstring>
#include <ostream>
#include "src\utils\PlayerTask.h"
#pragma warning(disable: 4190)

#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"

/*
Official Shoot converted to the testskill DLL entry style.
Enable this file with #if 1, and keep every other cpp that exports
player_plan disabled with #if 0.
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== official shoot params ====================*/
const float OFFICIAL_SHOOT_CENTER_TO_MOUTH = 8.0f;
const float OFFICIAL_SHOOT_VISION_ERROR = 2.0f;
const float OFFICIAL_SHOOT_FAST_DIST = 1.0f;
const float OFFICIAL_SHOOT_ADJUST_BACK_DIST = 20.0f;
const float OFFICIAL_SHOOT_WAIT_TOUCH_DIST = static_cast<float>(BALL_SIZE / 2 + MAX_ROBOT_SIZE + 5);
const float OFFICIAL_SHOOT_MOUTH_ANGLE = static_cast<float>(PI / 6);
const float OFFICIAL_SHOOT_TOWARD_GOAL_ANGLE = static_cast<float>(PI / 4);
const float OFFICIAL_SHOOT_BALL_FRONT_ANGLE = static_cast<float>(PI / 3);
const float OFFICIAL_SHOOT_GET_BALL_EXTRA_DIST = 5.0f;
const float OFFICIAL_SHOOT_KICK_DIST_OFFSET = -2.5f;
const double OFFICIAL_SHOOT_KICK_POWER = 127.0;

enum OfficialShootMethod
{
	OfficialShoot_None,
	OfficialShoot_ChaseBall,
	OfficialShoot_WaitTouch,
	OfficialShoot_StopBall,
	OfficialShoot_AdjustDir,
	OfficialShoot_ShootBall
};

bool officialShootValidRobot(int robot_id)
{
	return robot_id >= 0 && robot_id < 6;
}

bool officialShootHasBall(const WorldModel* model, int robot_id, float dist_threshold, float angle_threshold)
{
	const point2f& player_pos = model->get_our_player_pos(robot_id);
	const point2f& ball_pos = model->get_ball_pos();
	const float player_dir = model->get_our_player_dir(robot_id);
	const point2f player_to_ball = ball_pos - player_pos;

	return player_to_ball.length() < dist_threshold &&
		fabs(Maths::normalizeAngle(player_dir - player_to_ball.angle())) < angle_threshold;
}

PlayerTask officialShootChaseBall(const WorldModel* model, int robot_id)
{
	PlayerTask task;
	const point2f& ball_pos = model->get_ball_pos();
	const point2f opponent_goal = -FieldPoint::Goal_Center_Point;
	const float face_dir = (opponent_goal - ball_pos).angle();

	/* Simple replacement for official GetBall: stand behind the ball. */
	task.target_pos = ball_pos - Maths::vector2polar(OFFICIAL_SHOOT_ADJUST_BACK_DIST, face_dir);
	task.orientate = face_dir;
	task.needCb = true;
	return task;
}

PlayerTask officialShootWaitTouch(const WorldModel* model, int robot_id)
{
	PlayerTask task;
	const point2f& player_pos = model->get_our_player_pos(robot_id);
	const float player_dir = model->get_our_player_dir(robot_id);

	/* Simple replacement for official Halt: hold position and wait. */
	task.target_pos = player_pos;
	task.orientate = player_dir;
	task.needCb = true;

	if (officialShootHasBall(model, robot_id, OFFICIAL_SHOOT_WAIT_TOUCH_DIST, OFFICIAL_SHOOT_MOUTH_ANGLE))
	{
		task.kickPower = OFFICIAL_SHOOT_KICK_POWER;
		task.needKick = true;
		task.isChipKick = false;
	}

	return task;
}

PlayerTask officialShootStopBall(const WorldModel* model, int robot_id)
{
	PlayerTask task;
	const point2f& player_pos = model->get_our_player_pos(robot_id);
	const point2f& ball_pos = model->get_ball_pos();
	const float player_dir = model->get_our_player_dir(robot_id);
	const bool ball_in_mouth_dir = fabs(Maths::normalizeAngle(player_dir - (ball_pos - player_pos).angle())) < OFFICIAL_SHOOT_MOUTH_ANGLE;

	if (!ball_in_mouth_dir)
	{
		task.target_pos = player_pos;
		task.orientate = (ball_pos - player_pos).angle();
	}
	else
	{
		task.target_pos = player_pos;
		task.orientate = player_dir;
		task.needCb = true;
	}

	return task;
}

PlayerTask officialShootAdjustDir(const WorldModel* model)
{
	PlayerTask task;
	const point2f& ball_pos = model->get_ball_pos();
	const point2f opponent_goal = -FieldPoint::Goal_Center_Point;

	task.target_pos = ball_pos + Maths::vector2polar(OFFICIAL_SHOOT_ADJUST_BACK_DIST, (ball_pos - opponent_goal).angle());
	task.orientate = (opponent_goal - ball_pos).angle();
	task.needCb = true;
	return task;
}

PlayerTask officialShootBall(const WorldModel* model, int robot_id)
{
	PlayerTask task;
	const point2f opponent_goal = -FieldPoint::Goal_Center_Point;
	const point2f& ball_pos = model->get_ball_pos();

	if (officialShootHasBall(model, robot_id, get_ball_threshold + OFFICIAL_SHOOT_KICK_DIST_OFFSET, OFFICIAL_SHOOT_MOUTH_ANGLE))
	{
		task.kickPower = OFFICIAL_SHOOT_KICK_POWER;
		task.needKick = true;
		task.isChipKick = false;
	}

	task.target_pos = ball_pos + Maths::vector2polar(OFFICIAL_SHOOT_FAST_DIST, (ball_pos - opponent_goal).angle());
	task.orientate = (opponent_goal - ball_pos).angle();
	task.needCb = true;
	task.flag = 1;
	return task;
}

PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	PlayerTask task;

	if (model == NULL || !officialShootValidRobot(robot_id))
	{
		return task;
	}

	static bool ball_moving_to_head[6] = { false, false, false, false, false, false };

	const point2f& player_pos = model->get_our_player_pos(robot_id);
	const point2f& ball_pos = model->get_ball_pos();
	const point2f& last_ball_pos = model->get_ball_pos(1);
	const float player_dir = model->get_our_player_dir(robot_id);
	const point2f opponent_goal = -FieldPoint::Goal_Center_Point;

	const float player_to_goal_angle_error = Maths::normalizeAngle((opponent_goal - player_pos).angle() - player_dir);
	const float player_to_ball_angle_error = Maths::normalizeAngle((ball_pos - player_pos).angle() - player_dir);
	const bool toward_opp_goal = fabs(player_to_goal_angle_error) < OFFICIAL_SHOOT_TOWARD_GOAL_ANGLE;
	const bool ball_front_head = fabs(player_to_ball_angle_error) < OFFICIAL_SHOOT_BALL_FRONT_ANGLE;

	const point2f ball_move_vec = ball_pos - last_ball_pos;
	const point2f head_middle = player_pos + Maths::vector2polar(OFFICIAL_SHOOT_CENTER_TO_MOUTH, player_dir);
	const point2f ball_to_head_vec = head_middle - ball_pos;

	if (ball_move_vec.length() < OFFICIAL_SHOOT_VISION_ERROR || ball_to_head_vec.length() < 0.001f)
	{
		ball_moving_to_head[robot_id] = false;
	}
	else
	{
		const float dot_value = ball_to_head_vec.x * ball_move_vec.x + ball_to_head_vec.y * ball_move_vec.y;
		float cos_value = dot_value / (ball_to_head_vec.length() * ball_move_vec.length());
		cos_value = Maths::clip(cos_value, -1.0f, 1.0f);
		const float move_to_head_angle = static_cast<float>(acos(cos_value));
		ball_moving_to_head[robot_id] = fabs(move_to_head_angle) < OFFICIAL_SHOOT_MOUTH_ANGLE;
	}

	const bool wait_touch = ball_moving_to_head[robot_id] && ball_front_head && toward_opp_goal;
	const bool stop_ball = ball_front_head && ball_moving_to_head[robot_id] && !toward_opp_goal;

	OfficialShootMethod method = OfficialShoot_None;
	if (wait_touch)
		method = OfficialShoot_WaitTouch;
	else if (stop_ball)
		method = OfficialShoot_StopBall;
	else if (!toward_opp_goal)
		method = OfficialShoot_AdjustDir;
	else if (officialShootHasBall(model, robot_id, get_ball_threshold + OFFICIAL_SHOOT_GET_BALL_EXTRA_DIST, OFFICIAL_SHOOT_MOUTH_ANGLE))
		method = OfficialShoot_ShootBall;
	else
		method = OfficialShoot_ChaseBall;

	switch (method)
	{
	case OfficialShoot_WaitTouch:
		task = officialShootWaitTouch(model, robot_id);
		break;
	case OfficialShoot_StopBall:
		task = officialShootStopBall(model, robot_id);
		break;
	case OfficialShoot_AdjustDir:
		task = officialShootAdjustDir(model);
		break;
	case OfficialShoot_ShootBall:
		task = officialShootBall(model, robot_id);
		break;
	case OfficialShoot_ChaseBall:
		task = officialShootChaseBall(model, robot_id);
		break;
	default:
		break;
	}

	return task;
}
#endif
