#include "..\src\utils\PlayerTask.h"
#include "..\src\utils\worldmodel.h"
#include "..\src\utils\maths.h"
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
const float REAL_AWAY_BALL_DISTANCE_X = 40.0f;
const float SIM_AWAY_BALL_DISTANCE_X = 20.0f;
const float BALL_STILL_MOVE_DISTANCE = 0.8f;
const float BALL_SIDE_MOUTH_DISTANCE = 14.0f;
const float BALL_SIDE_BACK_DISTANCE = 20.0f;
const float RECEIVER_SIDE_OFFSET_Y = 20.0f;
const float SHOOT_SIDE_OFFSET_Y = 35.0f;
const double GET_BALL_KICK_POWER = 50.0;


/*==================== 角度处理 ====================*/
float normalizeAngle(float angle)
{
	while (angle > PI) angle -= 2 * PI;
	while (angle < -PI) angle += 2 * PI;
	return angle;
}


/*==================== 判断车头是否朝向对方球门 ====================*/
bool isTowardOpponentGoal(float direction)
{
	return direction < PI / 2 && direction > -PI / 2;
}


/*==================== 寻找接球队员 ====================*/
int findReceiverRobotId(const WorldModel* model, int robot_id)
{
	if (model == NULL) {
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


/*==================== DLL 入口 ====================*/
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
	const float awayBallDistanceX = isSimulationMode ? SIM_AWAY_BALL_DISTANCE_X : REAL_AWAY_BALL_DISTANCE_X;

	int receiverRobotId = findReceiverRobotId(model, robot_id);
	if (receiverRobotId == -1) {
		receiverRobotId = robot_id;
	}

	// 获取执行拿球需要用到的场上信息。
	const point2f& ballPosition = model->get_ball_pos();
	const point2f& lastBallPosition = model->get_ball_pos(1);
	const point2f& receiverPosition = model->get_our_player_pos(receiverRobotId);
	const point2f& getBallRobotPosition = model->get_our_player_pos(robot_id);
	const point2f opponentGoal = -FieldPoint::Goal_Center_Point;
	const float receiverDirection = model->get_our_player_dir(receiverRobotId);
	const point2f receiverHeadPosition = receiverPosition + Maths::vector2polar(static_cast<float>(ROBOT_HEAD), receiverDirection);
	const float robotDirection = model->get_our_player_dir(robot_id);

	float receiverToBallDirection = (ballPosition - receiverPosition).angle();
	float opponentGoalToBallDirection = (ballPosition - opponentGoal).angle();
	float ballAwayOpponentGoal = (ballPosition - opponentGoal).length();
	float robotAwayBall = (getBallRobotPosition - ballPosition).length();
	float robotAwayOpponentGoal = (getBallRobotPosition - opponentGoal).length();
	float ballMovingDistance = (ballPosition - lastBallPosition).length();

	bool isRobotTowardOpponentGoal = isTowardOpponentGoal(robotDirection);
	bool isBallBehindRobot = ballAwayOpponentGoal + BALL_SIZE + MAX_ROBOT_SIZE > robotAwayOpponentGoal;
	bool isBallMoving = ballMovingDistance >= BALL_STILL_MOVE_DISTANCE;
	bool isRobotTowardBall = fabs(normalizeAngle((ballPosition - getBallRobotPosition).angle() - robotDirection)) < (PI / 2 - PI / 12);

	float ballMovingDirection = (ballPosition - lastBallPosition).angle();
	point2f ballWithVelocity = ballPosition + Maths::vector2polar(ballMovingDistance, ballMovingDirection);

	if (!isBallMoving)
	{
		ballWithVelocity = ballPosition;
	}

	float ballRobotDirectionError = normalizeAngle((ballPosition - getBallRobotPosition).angle() - robotDirection);
	bool isBallBesideRobotMouth = robotAwayBall < BALL_SIDE_MOUTH_DISTANCE &&
		fabs(ballRobotDirectionError) > PI / 4 &&
		fabs(ballRobotDirectionError) < PI / 2;

	if (receiverRobotId == robot_id)
	{
		// receiverRobotId 和 robot_id 是同一车时，拿球方向改为朝对方球门。
		bool isBallXBehindRobot = (ballPosition.x - 2) < getBallRobotPosition.x;
		bool isBallYBelowRobot = (ballPosition.y - 2) < getBallRobotPosition.y;

		if (!isBallXBehindRobot)
		{
			task.target_pos = ballWithVelocity + Maths::vector2polar(static_cast<float>(BALL_SIZE / 2 + MAX_ROBOT_SIZE + getBallBuffer), opponentGoalToBallDirection);
		}
		else
		{
			if (isBallYBelowRobot)
				task.target_pos.set(ballWithVelocity.x - awayBallDistanceX, ballWithVelocity.y + SHOOT_SIDE_OFFSET_Y);
			else
				task.target_pos.set(ballWithVelocity.x - awayBallDistanceX, ballWithVelocity.y - SHOOT_SIDE_OFFSET_Y);
		}

		task.orientate = (opponentGoal - ballPosition).angle();
	}
	else
	{
		// receiverRobotId 和 robot_id 不是同一车时，围绕球调整到适合传球的位置。
		bool areBothLeftOfBallX = (ballPosition.x - 2) < getBallRobotPosition.x && (ballPosition.x - 2) < receiverPosition.x;
		bool areBothRightOfBallX = (ballPosition.x - 2) > getBallRobotPosition.x && (ballPosition.x - 2) > receiverPosition.x;
		bool isRobotAboveBallY = (ballPosition.y - 2) < getBallRobotPosition.y;

		if (areBothRightOfBallX)
		{
			if (isRobotAboveBallY)
				task.target_pos.set(ballWithVelocity.x + awayBallDistanceX, ballWithVelocity.y + RECEIVER_SIDE_OFFSET_Y);
			else
				task.target_pos.set(ballWithVelocity.x + awayBallDistanceX, ballWithVelocity.y - RECEIVER_SIDE_OFFSET_Y);
		}
		else if (areBothLeftOfBallX)
		{
			if (isRobotAboveBallY)
				task.target_pos.set(ballWithVelocity.x - awayBallDistanceX, ballWithVelocity.y + RECEIVER_SIDE_OFFSET_Y);
			else
				task.target_pos.set(ballWithVelocity.x - awayBallDistanceX, ballWithVelocity.y - RECEIVER_SIDE_OFFSET_Y);
		}
		else
		{
			task.target_pos = ballWithVelocity + Maths::vector2polar(static_cast<float>(BALL_SIZE / 2 + MAX_ROBOT_SIZE + getBallBuffer), receiverToBallDirection);
		}

		task.orientate = (receiverHeadPosition - ballPosition).angle();
	}

	// 原官方代码这里使用了 angle_diff、dist_to_ball、my_pos，但变量缺失。
	// 按原意补为：当前车头方向接近任务朝向，并且小车已经靠近球。
	float targetDirectionError = normalizeAngle(static_cast<float>(task.orientate) - robotDirection);
	if (fabs(targetDirectionError) < 0.01f && robotAwayBall < get_ball_threshold)
	{
		task.needKick = true;
		task.isPass = true;
		task.isChipKick = false;
		task.kickPower = GET_BALL_KICK_POWER;
		task.target_pos = getBallRobotPosition;
		task.global_vel = point2f(0, 0);
		task.rot_vel = 0;
	}

	// 判断球是否在小车嘴侧面；如果是，让车后退一点重新拿球。
	if (isBallBesideRobotMouth)
	{
		task.target_pos = ballPosition + Maths::vector2polar(BALL_SIDE_BACK_DISTANCE, normalizeAngle(robotDirection + PI));
	}

	return task;
}
