#if 0
#include "src\utils\PlayerTask.h"
#include "src\getballsource.h"
#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"

/*
功能流程：
1. 本文件负责接球车站到固定接球点，并根据球是否离开发球队员切换等待/迎球状态。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 里会自动选择对应参数。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 接球调参区 ====================*/
const float REAL_RECEIVE_POINT_X = 20.0f;
const float REAL_RECEIVE_POINT_Y = 70.0f;
const float SIM_RECEIVE_POINT_X = 20.0f;
const float SIM_RECEIVE_POINT_Y = 70.0f;
const float REAL_BALL_LEAVE_DISTANCE_MARGIN = 8.0f;
const float SIM_BALL_LEAVE_DISTANCE_MARGIN = 8.0f;




/*==================== 功能块 1：角度处理 ====================*/
/*
将角度限制在 [-PI, PI] 范围内，避免方向计算时出现角度跳变。
*/
float normalizeAngleToPiRange(float angle)
{
	while (angle > PI) angle -= 2 * PI;
	while (angle < -PI) angle += 2 * PI;
	return angle;
}


PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	/*==================== 功能块 2：初始化任务 ====================*/
	/*
	创建任务对象，并判断视觉/世界模型数据是否有效。，明确里一台
	*/
	PlayerTask receiveTask;

	if (model == NULL) {
		return receiveTask;
	}

	const bool isSimulationMode = model != NULL && model->get_simulation();
	const float receivePointX = isSimulationMode ? SIM_RECEIVE_POINT_X : REAL_RECEIVE_POINT_X;
	const float receivePointY = isSimulationMode ? SIM_RECEIVE_POINT_Y : REAL_RECEIVE_POINT_Y;
	const float ballLeaveDistanceMargin = isSimulationMode ? SIM_BALL_LEAVE_DISTANCE_MARGIN : REAL_BALL_LEAVE_DISTANCE_MARGIN;

	int kickerRobotId = -1;

	for (int i = 0; i < 6; i++)
	{
		if (i == robot_id || i == model->get_our_goalie())
			continue;

		if (model->get_our_exist_id()[i])
			kickerRobotId = i;
	}


	/*==================== 功能块 3：获取场上信息 ====================*/
	/*
	获取机器人当前位置、球的位置和球的速度。
	receivePoint 当前写法等于固定接球点，相当于原地等待点。
	*/
	point2f receivePoint(receivePointX, receivePointY);

	const point2f& receiverRobotPosition = model->get_our_player_pos(robot_id);
	const point2f& ballPosition = model->get_ball_pos();
	const point2f& ballVelocity = model->get_ball_vel();
	

	/*==================== 功能块 4：设置默认任务 ====================*/
	/*
	默认目标点为接球点，默认开启吸球，不主动踢球,角度是接球车到球的方向
	*/
	receiveTask.target_pos = receivePoint;//可以要把这个给他改为固定点位



	receiveTask.needCb = true;
	receiveTask.needKick = false;
	receiveTask.isPass = false;

	float faceDirection = 0.0f;
	faceDirection = (ballPosition - receiverRobotPosition).angle();
	receiveTask.orientate = faceDirection;
	/*==================== 功能块 5：判断球是否传来 ====================*/
	
	//根据球离发球球员的距离来判断
   
	const point2f& kickerRobotPosition = model->get_our_player_pos(kickerRobotId);

	// 球到踢球球员的距离
	float ballToKickerDistance = (ballPosition - kickerRobotPosition).length();

	// 当球离开踢球球员一定距离，认为球已经被传出来
	bool isBallComing = ballToKickerDistance > MAX_ROBOT_SIZE + ballLeaveDistanceMargin;
	/*==================== 功能块 6：动态接球 ====================*/
	/*
	球在运动时：
	根据球的运动方向计算接球路线，
	让小车移动到球路附近，并面向来球方向接球。
	*/
	if (isBallComing)
	{
		float ballMoveDirection = ballVelocity.angle();

		//	point2f interceptPoint = Maths::line_perp_across(
		//	ballPosition,
		//	ballMoveDirection,
		//	receivePoint
	//};





//这个应该要修改
		//receiveTask.target_pos =
		//	interceptPoint;// + Maths::vector2polar(MAX_ROBOT_SIZE + 3.0f, ballMoveDirection);
//
		faceDirection = normalizeAngleToPiRange(ballMoveDirection + PI);
	}


	/*==================== 功能块 7：等待接球 ====================*/
	/*
	球没有明显运动时：
	小车停在接球点等待，并保持车头朝向小球。
	*/
	else
	{
		receiveTask.target_pos = receivePoint;
		faceDirection = (ballPosition - receiverRobotPosition).angle();
	}


	/*==================== 功能块 8：输出任务 ====================*/
	/*
	设置最终车头方向，并返回任务给系统执行。
	*/
	receiveTask.orientate = faceDirection;

	return receiveTask;
}
#endif
