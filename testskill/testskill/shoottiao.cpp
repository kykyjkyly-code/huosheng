#if 0
#include"src\utils\PlayerTask.h"
#include"src\getballsource.h"
#include"src\utils\worldmodel.h"
#include"src\utils\maths.h"
#include <cmath>

/*
功能流程：
1. 本文件负责挑射：先找到拿球方向，控到球后执行挑射。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 和 isget 会自动选择对应参数。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 挑射调参区 ====================*/
// 球在车头方向的最大角度误差，单位是弧度。
const float REAL_SHOOTTIAO_MOUTH_ANGLE_THRESHOLD = 0.05f;
const float SIM_SHOOTTIAO_MOUTH_ANGLE_THRESHOLD = 0.05f;
// 默认拿球时站在球后方的距离。
const float REAL_SHOOTTIAO_GET_BALL_BACK_DIST = 13.0f;
const float SIM_SHOOTTIAO_GET_BALL_BACK_DIST = 13.0f;
// 控到球后贴近球的位置距离。
const float REAL_SHOOTTIAO_READY_BACK_DIST = 1.0f;
const float SIM_SHOOTTIAO_READY_BACK_DIST = 1.0f;
// 没控到球时继续靠近球的距离。
const float REAL_SHOOTTIAO_APPROACH_BACK_DIST = 5.0f;
const float SIM_SHOOTTIAO_APPROACH_BACK_DIST = 5.0f;
// 挑射力度。
const double REAL_SHOOTTIAO_KICK_POWER = 30.0;
const double SIM_SHOOTTIAO_KICK_POWER = 70.0;


//==================== 功能块 1：判断是否控到球 ====================
// 根据球和小车的位置、距离、方向，判断球是否已经在控球嘴附近。
bool isget(const WorldModel* model, int robot_id)
{
	if (model == NULL) {
		return false;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float mouth_angle_threshold = is_sim ? SIM_SHOOTTIAO_MOUTH_ANGLE_THRESHOLD : REAL_SHOOTTIAO_MOUTH_ANGLE_THRESHOLD;

	// 获取球员坐标
	const point2f& player_pos = model->get_our_player_pos(robot_id);

	// 获取球的位置
	const point2f& ball_pos = model->get_ball_pos();

	// 获取球员朝向
	const float my_dir = model->get_our_player_dir(robot_id);

	// 小车到球的向量
	const point2f player_to_ball = ball_pos - player_pos;

	// 小车到球的距离
	const float ball_dist = player_to_ball.length();

	// 小车指向球的方向
	const float ball_dir = player_to_ball.angle();

	// 车头方向和球方向的角度差
	const float dir_error = fabs(ball_dir - my_dir);

	// 角度阈值：球必须在车头前方
	

	// 判断球是否离小车足够近
	const bool ball_near = ball_dist < get_ball_threshold;

	// 判断球是否在小车车头方向
	const bool ball_in_front = dir_error < mouth_angle_threshold;

	return ball_near && ball_in_front;
}


//==================== 功能块 2：主函数初始化 ====================
// 创建任务对象，判断世界模型是否有效。
PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	PlayerTask task;

	if (model == NULL) {
		return task;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float get_ball_back_dist = is_sim ? SIM_SHOOTTIAO_GET_BALL_BACK_DIST : REAL_SHOOTTIAO_GET_BALL_BACK_DIST;
	const float ready_back_dist = is_sim ? SIM_SHOOTTIAO_READY_BACK_DIST : REAL_SHOOTTIAO_READY_BACK_DIST;
	const float approach_back_dist = is_sim ? SIM_SHOOTTIAO_APPROACH_BACK_DIST : REAL_SHOOTTIAO_APPROACH_BACK_DIST;
	const double kick_power = is_sim ? SIM_SHOOTTIAO_KICK_POWER : REAL_SHOOTTIAO_KICK_POWER;


	//==================== 功能块 3：寻找接球队友 ====================
	// 从我方机器人中选择一台非自己、非守门员的车作为接球队员。
	int receiver_id = -1;

	for (int i = 0; i < 6; i++)
	{
		if (i == robot_id || i == model->get_our_goalie())
			continue;

		if (model->get_our_exist_id()[i])
		{
			receiver_id = i;
			break;
		}
	}


	//==================== 功能块 4：没有接球队友时的处理 ====================
	// 如果没有找到接球队员，就直接返回空任务。
	if (receiver_id == -1)
	{
		return task;
	}


	//==================== 功能块 5：获取场上关键信息 ====================
	// 获取当前小车、球、接球队友、球门的位置。
	const point2f& player_pos = model->get_our_player_pos(robot_id);
	const point2f& ball_pos = model->get_ball_pos();
	const point2f& receiver_pos = model->get_our_player_pos(receiver_id);

	// 敌方球门中心
	point2f goal = -FieldPoint::Goal_Center_Point;


	//==================== 功能块 6：计算挑射方向 ====================
	// 挑射方向为：球指向敌方球门。
	float face_dir = (goal - ball_pos).angle();


	//==================== 功能块 7：设置默认拿球任务 ====================
	// 默认去球后方，打开吸球，暂时不踢球。
	task.orientate = face_dir;
	task.target_pos = ball_pos - Maths::vector2polar(get_ball_back_dist, face_dir);

	task.needCb = true;
	task.needKick = false;
	task.isPass = false;


	//==================== 功能块 8：控到球后的挑射逻辑 ====================
	// 如果球已经在控球嘴上，就朝球门方向挑射。
	if (isget(model, robot_id))
	{
		// 贴近球，保持朝向球门方向
		task.target_pos = ball_pos - Maths::vector2polar(ready_back_dist, face_dir);
		task.orientate = face_dir;

		// 打开吸球
		task.needCb = true;

		// 开启挑射
		task.isChipKick = true;

		// 开启击球
		task.needKick = true;

		// 这是射门，不是传球
		task.isPass = false;

		// 挑射力度，可以根据距离调整
		task.kickPower = kick_power;
	}


	//==================== 功能块 9：未控到球时继续拿球 ====================
	// 如果还没控到球，小车继续去球后方吸球。
	else
	{
		task.orientate = face_dir;

		// 小一点更贴球，大一点更保守
		task.target_pos = ball_pos - Maths::vector2polar(approach_back_dist, face_dir);

		task.needCb = true;
		task.needKick = false;
		task.isPass = false;
	}


	//==================== 功能块 10：返回任务 ====================
	return task;
}
#endif
