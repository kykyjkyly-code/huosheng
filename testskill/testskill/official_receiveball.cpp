#if 0
#include <cmath>
#include <cstring>
#include <ostream>
#include "src\utils\PlayerTask.h"
#pragma warning(disable: 4190)

#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"

/*
Official ReceiveBall converted to the testskill DLL entry style.
Enable this file with #if 1, and keep every other cpp that exports
player_plan disabled with #if 0.
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== official receive params ====================*/
const float OFFICIAL_RECEIVE_BALL_VISION_ERROR = 2.5f;
const float OFFICIAL_RECEIVE_CLOSE_DIST = 50.0f;
const float OFFICIAL_RECEIVE_HEAD_TOWARD_BALL_ANGLE = static_cast<float>(5 * PI / 12);
const float OFFICIAL_RECEIVE_BALL_TO_HEAD_ANGLE = static_cast<float>(PI / 6);
const float OFFICIAL_RECEIVE_FIELD_MARGIN = 8.0f;

bool officialReceiveValidRobot(int robot_id)
{
	return robot_id >= 0 && robot_id < 6;
}

point2f officialReceiveClipToField(const point2f& p)
{
	point2f clipped = p;
	clipped.x = Maths::clip(clipped.x, static_cast<float>(-FIELD_LENGTH_H + OFFICIAL_RECEIVE_FIELD_MARGIN), static_cast<float>(FIELD_LENGTH_H - OFFICIAL_RECEIVE_FIELD_MARGIN));
	clipped.y = Maths::clip(clipped.y, static_cast<float>(-FIELD_WIDTH_H + OFFICIAL_RECEIVE_FIELD_MARGIN), static_cast<float>(FIELD_WIDTH_H - OFFICIAL_RECEIVE_FIELD_MARGIN));
	return clipped;
}

PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	PlayerTask task;

	if (model == NULL || !officialReceiveValidRobot(robot_id))
	{
		return task;
	}

	const point2f& ball_pos = model->get_ball_pos();
	const point2f& ball_vel = model->get_ball_vel();
	const point2f& receiver_pos = model->get_our_player_pos(robot_id);
	const point2f opponent_goal = -FieldPoint::Goal_Center_Point;
	const float receiver_dir = model->get_our_player_dir(robot_id);
	const point2f receiver_head = receiver_pos + Maths::vector2polar(static_cast<float>(ROBOT_HEAD), receiver_dir);

	const bool close_receiver = (receiver_pos - ball_pos).length() < OFFICIAL_RECEIVE_CLOSE_DIST;
	const bool head_toward_ball = fabs(Maths::normalizeAngle((ball_pos - receiver_head).angle() - receiver_dir)) < OFFICIAL_RECEIVE_HEAD_TOWARD_BALL_ANGLE;
	const bool ball_move_to_head = ball_vel.length() >= OFFICIAL_RECEIVE_BALL_VISION_ERROR &&
		fabs(Maths::normalizeAngle((receiver_head - ball_pos).angle() - ball_vel.angle())) < OFFICIAL_RECEIVE_BALL_TO_HEAD_ANGLE;

	task.needCb = true;
	task.needKick = false;
	task.isPass = false;

	if (ball_vel.length() < OFFICIAL_RECEIVE_BALL_VISION_ERROR)
	{
		task.target_pos = receiver_pos;
		task.orientate = (ball_pos - receiver_pos).angle();
		return task;
	}

	/* Same call shape as the official source. */
	point2f task_point = Maths::line_perp_across(ball_pos, ball_vel.angle(), receiver_pos);

	if (close_receiver && head_toward_ball && ball_move_to_head)
	{
		task_point = receiver_pos;
	}

	task.target_pos = officialReceiveClipToField(task_point);
	task.orientate = (opponent_goal - receiver_pos).angle();
	return task;
}
#endif
