#include "..\src\utils\PlayerTask.h"
#include "..\src\utils\worldmodel.h"
#include "..\src\utils\maths.h"
#include <cmath>

/*
功能流程：
1. 本文件由官方 PassBall::plan 改成 testskill 里可导出 DLL 的 player_plan 形式。
2. 原 pass 逻辑保持不变：判断传球路线、判断是否控球，然后执行传球或继续调整位置。
3. DLL 入口只有 robot_id，没有 receiver_id，所以这里自动选择一台非自己、非守门员的我方车作为接球队员。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 官方传球参数 ====================*/
const float PASS_RECEIVER_HEAD_LENGTH = 7.0f;
const float PASS_FAST_APPROACH_DISTANCE = 3.0f;
const float PASS_READY_ANGLE_THRESHOLD = 0.5f;
const float PASS_GET_BALL_DISTANCE_OFFSET = -1.5f;
const float PASS_GET_BALL_ANGLE_THRESHOLD = PI / 6;
const float PASS_NOT_READY_BACK_DISTANCE = static_cast<float>(BALL_SIZE / 2 + MAX_ROBOT_SIZE + 12);
const double PASS_KICK_POWER = 127.0;


/*==================== 角度处理 ====================*/
float normalizeAngle(float angle)
{
	while (angle > PI) angle -= 2 * PI;
	while (angle < -PI) angle += 2 * PI;
	return angle;
}


/*==================== 判断是否可以传球 ====================*/
bool isReadyPass(const point2f& ballPosition, const point2f& passerPosition, const point2f& receiverPosition)
{
	// 接球车到球矢量角度
	float receiverToBallDirection = (ballPosition - receiverPosition).angle();

	// 球到传球车矢量角度
	float ballToPasserDirection = (passerPosition - ballPosition).angle();

	// 两个矢量角度之差小于某个值，判断是否可以传球
	bool canPass = fabs(receiverToBallDirection - ballToPasserDirection) < PASS_READY_ANGLE_THRESHOLD;

	return canPass;
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

	int receiverRobotId = findReceiverRobotId(model, robot_id);
	if (receiverRobotId == -1) {
		receiverRobotId = robot_id;
	}

	// 获取执行传球需要用到的参数。
	const float receiverDirection = model->get_our_player_dir(receiverRobotId);
	const float passerDirection = model->get_our_player_dir(robot_id);
	const point2f& receiverPosition = model->get_our_player_pos(receiverRobotId);
	const point2f& passerPosition = model->get_our_player_pos(robot_id);
	const point2f& ballPosition = model->get_ball_pos();
	const point2f opponentGoal = -FieldPoint::Goal_Center_Point;

	float receiverToBallDirection = (ballPosition - receiverPosition).angle();
	point2f receiverHeadPosition = receiverPosition + Maths::vector2polar(PASS_RECEIVER_HEAD_LENGTH, receiverDirection);
	float passDirection = (receiverHeadPosition - ballPosition).angle();

	// 判断球是否在小车控球嘴上：
	// 1. ball 到车的距离足够近。
	// 2. 车头方向和车到球矢量角度之差足够小。
	bool hasBall = (ballPosition - passerPosition).length() < get_ball_threshold + PASS_GET_BALL_DISTANCE_OFFSET &&
		fabs(normalizeAngle(passerDirection - (ballPosition - passerPosition).angle())) < PASS_GET_BALL_ANGLE_THRESHOLD;

	// 如果 receiverRobotId 和 robot_id 是同一车，则直接射门。
	if (receiverRobotId == robot_id)
	{
		if (hasBall)
		{
			task.kickPower = PASS_KICK_POWER;
			task.needKick = true;
			task.isChipKick = false;
		}

		float opponentGoalToBallDirection = (ballPosition - opponentGoal).angle();
		task.target_pos = ballPosition + Maths::vector2polar(PASS_FAST_APPROACH_DISTANCE, opponentGoalToBallDirection);
		task.orientate = (opponentGoal - ballPosition).angle();
		return task;
	}

	// 判断并执行传球。
	if (isReadyPass(ballPosition, passerPosition, receiverPosition))
	{
		if (hasBall)
		{
			task.kickPower = PASS_KICK_POWER;
			task.needKick = true;
			task.isChipKick = false;
		}

		task.target_pos = ballPosition + Maths::vector2polar(PASS_FAST_APPROACH_DISTANCE, receiverToBallDirection);
	}
	else
	{
		task.target_pos = ballPosition + Maths::vector2polar(PASS_NOT_READY_BACK_DISTANCE, receiverToBallDirection);
	}

	task.orientate = passDirection;

	// flag = 1 表示小车加速度 * 2。
	task.flag = 1;

	return task;
}
