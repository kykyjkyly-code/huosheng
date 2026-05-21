﻿#include "src\utils\PlayerTask.h"
#pragma warning(disable: 4190)

#include "src\getballsource.h"
#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"
#include <cmath>


/*
功能流程：
1. 本文件由官方 GetBall::plan 改成 testskill 里可导出 DLL 的 player_plan 形式。
2. 原拿球逻辑保持不变：根据球、拿球队员、接球队员的位置，选择绕球/靠近球的目标点。
3. DLL 入口只有 robot_id，没有 receiver_id，所以这里自动选择一台非自己、非守门员的我方车作为接球队员。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 官方拿球参数 ====================*/
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


/*==================== 官方拿球参数 ====================*/
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

	int receiverId = Maths::findReceiverRobotId(model, robot_id);
	if (receiverId == -1) {
		receiverId = robot_id;
	}

	// 获取执行拿球需要用到的场上信息。
	const point2f& ballPos = model->get_ball_pos();
	const point2f& lastBallPos = model->get_ball_pos(1);
	const point2f& receiverPos = model->get_our_player_pos(receiverId);
	const point2f& myPos = model->get_our_player_pos(robot_id);
	const point2f opponentGoal = -FieldPoint::Goal_Center_Point;
	const float receiverDir = model->get_our_player_dir(receiverId);
	const point2f receiverFront = receiverPos + Maths::vector2polar(static_cast<float>(ROBOT_HEAD), receiverDir);
	const float myDir = model->get_our_player_dir(robot_id);

	float recvToBallAngle = (ballPos - receiverPos).angle();
	float goalToBallAngle = (ballPos - opponentGoal).angle();
	float ballToGoalDist = (ballPos - opponentGoal).length();
	float distToBall = (myPos - ballPos).length();
	float distToGoal = (myPos - opponentGoal).length();
	float ballMoveDist = (ballPos - lastBallPos).length();

	bool isFacingGoal = Maths::isTowardOpponentGoal(myDir);
	bool isBallBehindMe = ballToGoalDist + BALL_SIZE + MAX_ROBOT_SIZE > distToGoal;
	bool isBallMoving = ballMoveDist >= BALL_STILL_MOVE_DISTANCE;
	bool isFacingBall = fabs(Maths::normalizeAngle((ballPos - myPos).angle() - myDir)) < static_cast<float>(PI / 2 - PI / 12);

	float ballMoveAngle = (ballPos - lastBallPos).angle();
	point2f ballPredicted = ballPos + Maths::vector2polar(ballMoveDist, ballMoveAngle);

	if (!isBallMoving)
	{
		ballPredicted = ballPos;
	}

	float ballAngleErr = Maths::normalizeAngle((ballPos - myPos).angle() - myDir);
	bool isBallBesideMe = distToBall < BALL_SIDE_MOUTH_DISTANCE &&
		fabs(ballAngleErr) > static_cast<float>(PI / 4) &&
		fabs(ballAngleErr) < static_cast<float>(PI / 2);

	if (receiverId == robot_id)
	{
		// receiverId 和 robot_id 是同一车时，拿球方向改为朝对方球门。
		bool isBallBehindX = (ballPos.x - 2) < myPos.x;
		bool isBallBelowY = (ballPos.y - 2) < myPos.y;

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
		// receiverId 和 robot_id 不是同一车时，围绕球调整到适合传球的位置。
		bool bothLeftOfBall = (ballPos.x - 2) < myPos.x && (ballPos.x - 2) < receiverPos.x;
		bool bothRightOfBall = (ballPos.x - 2) > myPos.x && (ballPos.x - 2) > receiverPos.x;
		bool isMeAboveBall = (ballPos.y - 2) < myPos.y;

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

	// 原官方代码这里使用了 angle_diff、dist_to_ball、my_pos，但变量缺失。
	// 按原意补为：当前车头方向接近任务朝向，并且小车已经靠近球。
	float angleErr = Maths::normalizeAngle(static_cast<float>(task.orientate) - myDir);
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

	// 判断球是否在小车嘴侧面；如果是，让车后退一点重新拿球。
	if (isBallBesideMe)
	{
		task.target_pos = ballPos + Maths::vector2polar(BALL_SIDE_BACK_DISTANCE, Maths::normalizeAngle(myDir + static_cast<float>(PI)));
	}

	return task;
}
