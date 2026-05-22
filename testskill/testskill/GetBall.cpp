#if 0
#include <cmath>
#include <cstring>
#include <ostream>
#include "src\utils\PlayerTask.h"
#pragma warning(disable: 4190)

#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"

/*
Official GetBall converted to the testskill DLL entry style.
This file is currently active and exports player_plan.
Keep other cpp files that export player_plan disabled with #if 0.
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== official get-ball params ====================*/
const float REAL_GET_BALL_BUFFER = 5.0f;
const float SIM_GET_BALL_BUFFER = -4.0f;
const float REAL_OFFSET_X = 40.0f;
const float SIM_OFFSET_X = 20.0f;
const float BALL_STILL_MOVE_DISTANCE = 0.8f;
const float BALL_SIDE_MOUTH_DISTANCE = 14.0f;
const float BALL_SIDE_BACK_DISTANCE = 20.0f;
const float RECEIVER_SIDE_OFFSET_Y = 20.0f;
const float SHOOT_SIDE_OFFSET_Y = 35.0f;
const double GET_BALL_KICK_POWER = 50.0;

int findReceiverRobotId(const WorldModel* model, int robot_id)
{
	if (model == NULL || model->get_our_exist_id() == NULL) {
		return -1;
	}

	for (int i = 0; i < 6; i++)
	{
		if (i == robot_id || i == model->get_our_goalie())
			continue;

		if (model->get_our_exist_id()[i])
			return i;
	}

	return -1;
}

PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	PlayerTask task;

	if (model == NULL) {
		return task;
	}

	if (robot_id < 0 || robot_id >= 6) {
		return task;
	}

	const bool isSimulationMode = model->get_simulation();
	const float getBallBuffer = isSimulationMode ? SIM_GET_BALL_BUFFER : REAL_GET_BALL_BUFFER;
	const float offsetX = isSimulationMode ? SIM_OFFSET_X : REAL_OFFSET_X;

	int receiverId = findReceiverRobotId(model, robot_id);
	if (receiverId == -1) {
		receiverId = robot_id;
	}

	const point2f& ballPos = model->get_ball_pos();
	const point2f& lastBallPos = model->get_ball_pos(1);
	const point2f& receiverPos = model->get_our_player_pos(receiverId);
	const point2f& myPos = model->get_our_player_pos(robot_id);
	const point2f opponentGoal = -FieldPoint::Goal_Center_Point;
	const float receiverDir = model->get_our_player_dir(receiverId);
	const point2f receiverFront = receiverPos + Maths::vector2polar(static_cast<float>(ROBOT_HEAD), receiverDir);
	const float myDir = model->get_our_player_dir(robot_id);

	const float recvToBallAngle = (ballPos - receiverPos).angle();
	const float goalToBallAngle = (ballPos - opponentGoal).angle();
	const float ballToGoalDist = (ballPos - opponentGoal).length();
	const float distToBall = (myPos - ballPos).length();
	const float distToGoal = (myPos - opponentGoal).length();
	const float ballMoveDist = (ballPos - lastBallPos).length();

	const bool isBallMoving = ballMoveDist >= BALL_STILL_MOVE_DISTANCE;
	const bool isBallBehindMe = ballToGoalDist + BALL_SIZE + MAX_ROBOT_SIZE > distToGoal;
	const bool isFacingBall = fabs(Maths::normalizeAngle((ballPos - myPos).angle() - myDir)) < static_cast<float>(PI / 2 - PI / 12);

	float ballMoveAngle = (ballPos - lastBallPos).angle();
	point2f ballPredicted = ballPos + Maths::vector2polar(ballMoveDist, ballMoveAngle);

	if (!isBallMoving)
	{
		ballPredicted = ballPos;
	}

	const float ballAngleErr = Maths::normalizeAngle((ballPos - myPos).angle() - myDir);
	const bool isBallBesideMe = distToBall < BALL_SIDE_MOUTH_DISTANCE &&
		fabs(ballAngleErr) > static_cast<float>(PI / 4) &&
		fabs(ballAngleErr) < static_cast<float>(PI / 2);

	if (receiverId == robot_id)
	{
		const bool isBallBehindX = (ballPos.x - 2) < myPos.x;
		const bool isBallBelowY = (ballPos.y - 2) < myPos.y;

		if (!isBallBehindX)
		{
			task.target_pos = ballPredicted + Maths::vector2polar(static_cast<float>(BALL_SIZE / 2 + MAX_ROBOT_SIZE + getBallBuffer), goalToBallAngle);
		}
		else
		{
			if (isBallBelowY)
				task.target_pos.set(ballPredicted.x - offsetX, ballPredicted.y + SHOOT_SIDE_OFFSET_Y);
			else
				task.target_pos.set(ballPredicted.x - offsetX, ballPredicted.y - SHOOT_SIDE_OFFSET_Y);
		}

		task.orientate = (opponentGoal - ballPos).angle();
	}
	else
	{
		const bool bothLeftOfBall = (ballPos.x - 2) < myPos.x && (ballPos.x - 2) < receiverPos.x;
		const bool bothRightOfBall = (ballPos.x - 2) > myPos.x && (ballPos.x - 2) > receiverPos.x;
		const bool isMeAboveBall = (ballPos.y - 2) < myPos.y;

		if (bothRightOfBall)
		{
			if (isMeAboveBall)
				task.target_pos.set(ballPredicted.x + offsetX, ballPredicted.y + RECEIVER_SIDE_OFFSET_Y);
			else
				task.target_pos.set(ballPredicted.x + offsetX, ballPredicted.y - RECEIVER_SIDE_OFFSET_Y);
		}
		else if (bothLeftOfBall)
		{
			if (isMeAboveBall)
				task.target_pos.set(ballPredicted.x - offsetX, ballPredicted.y + RECEIVER_SIDE_OFFSET_Y);
			else
				task.target_pos.set(ballPredicted.x - offsetX, ballPredicted.y - RECEIVER_SIDE_OFFSET_Y);
		}
		else
		{
			task.target_pos = ballPredicted + Maths::vector2polar(static_cast<float>(BALL_SIZE / 2 + MAX_ROBOT_SIZE + getBallBuffer), recvToBallAngle);
		}

		task.orientate = (receiverFront - ballPos).angle();
	}

	const float angleErr = Maths::normalizeAngle(static_cast<float>(task.orientate) - myDir);
	if (fabs(angleErr) < 0.01f && distToBall < get_ball_threshold)
	{
		task.needKick = true;
		task.isPass = true;
		task.isChipKick = false;
		task.kickPower = GET_BALL_KICK_POWER;
		task.target_pos = myPos;
		task.global_vel = point2f(0, 0);
		task.rot_vel = 0;
	}

	if (isBallBesideMe)
	{
		task.target_pos = ballPos + Maths::vector2polar(BALL_SIDE_BACK_DISTANCE, Maths::normalizeAngle(myDir + static_cast<float>(PI)));
	}

	task.needCb = true;
	task.flag = 1;

	/* Keep these checks live so tuning can reuse them later. */
	if (isBallBehindMe && isFacingBall) {
		task.needCb = true;
	}

return task;
}
#endif
