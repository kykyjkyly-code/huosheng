#if 0
#include "src\utils\PlayerTask.h"
#include "src\getballsource.h"
#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"
#include <cmath>


/*
功能流程：
1. 本文件负责守门：判断球是否进禁区，能解围就挑球，不解围就站到球门防守点。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 里会自动选择对应参数。
*/

#pragma warning(push)
#pragma warning(disable:4190)
extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);
#pragma warning(pop)

/*==================== 守门调参区 ====================*/
// 球离守门员多近时允许挑球解围。
const float REAL_SHOU_MEN_CLEAR_BALL_DIST_EXTRA = 2.0f;
const float SIM_SHOU_MEN_CLEAR_BALL_DIST_EXTRA = 2.0f;
// 守门员挑球前的车头角度误差阈值。
const float REAL_SHOU_MEN_CLEAR_ANGLE_THRESHOLD = static_cast<float>(PI / 6);
const float SIM_SHOU_MEN_CLEAR_ANGLE_THRESHOLD = static_cast<float>(PI / 6);
// 守门员挑球解围力度。
const double REAL_SHOU_MEN_CLEAR_KICK_POWER = 127.0;
const double SIM_SHOU_MEN_CLEAR_KICK_POWER = 127.0;
// 球在禁区内时，守门员靠近球的距离修正。
const float REAL_SHOU_MEN_ACTIVE_TARGET_EXTRA = -2.0f;
const float SIM_SHOU_MEN_ACTIVE_TARGET_EXTRA = -2.0f;
// 球不在禁区内时，守门员站在球门前的距离。
const float REAL_SHOU_MEN_STAND_DIST = 30.0f;
const float SIM_SHOU_MEN_STAND_DIST = 30.0f;
// 守门员目标点平滑系数，越大越稳但越慢。
const float REAL_SHOU_MEN_SMOOTH = 0.88f;
const float SIM_SHOU_MEN_SMOOTH = 0.88f;






/*====================【功能块 1：判断球是否在我方禁区内】====================*/
/*
这个函数吸收了官方 Goalie::is_inside_penalty 的逻辑。

禁区大致分成三块：
1. 中间矩形区域
2. 左侧圆弧区域
3. 右侧圆弧区域

参数 p：
一般传入球的位置 ball。

返回值：
true  ：球在我方禁区内
false ：球不在我方禁区内
*/
bool is_inside_penalty(const point2f& p)
{
	// 禁区左侧圆弧中心点
	const point2f& a = FieldPoint::Penalty_Arc_Center_Left;

	// 禁区右侧圆弧中心点
	const point2f& b = FieldPoint::Penalty_Arc_Center_Right;


	/*====================【功能块 1.1：判断中间矩形禁区】====================*/
	if (fabs(p.y) < PENALTY_AREA_L / 2)
	{
		return p.x < -FIELD_LENGTH_H + PENALTY_AREA_R &&
			p.x > -FIELD_LENGTH_H;
	}

	/*====================【功能块 1.2：判断左侧圆弧禁区】====================*/
	else if (p.y < 0)
	{
		return (p - a).length() < PENALTY_AREA_R &&
			p.x > -FIELD_LENGTH_H;
	}

	/*====================【功能块 1.3：判断右侧圆弧禁区】====================*/
	else
	{
		return (p - b).length() < PENALTY_AREA_R &&
			p.x > -FIELD_LENGTH_H;
	}
}


/*====================【功能块 2：守门员主函数】====================*/
/*
player_plan 是 SOM C++ 二次开发 DLL 的入口函数。

model：
比赛信息对象，通过它获取球、机器人位置、机器人朝向。

robot_id：
当前执行这个 DLL 技能的机器人编号。

返回值 task：
告诉系统这个机器人要去哪里、朝哪里、是否踢球。
*/
PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	/*====================【功能块 2.1：创建任务对象与安全判断】====================*/
	PlayerTask task;

	if (model == NULL)
	{
		return task;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float clear_ball_dist_extra = is_sim ? SIM_SHOU_MEN_CLEAR_BALL_DIST_EXTRA : REAL_SHOU_MEN_CLEAR_BALL_DIST_EXTRA;
	const float clear_angle_threshold = is_sim ? SIM_SHOU_MEN_CLEAR_ANGLE_THRESHOLD : REAL_SHOU_MEN_CLEAR_ANGLE_THRESHOLD;
	const double clear_kick_power = is_sim ? SIM_SHOU_MEN_CLEAR_KICK_POWER : REAL_SHOU_MEN_CLEAR_KICK_POWER;
	const float active_target_extra = is_sim ? SIM_SHOU_MEN_ACTIVE_TARGET_EXTRA : REAL_SHOU_MEN_ACTIVE_TARGET_EXTRA;
	const float stand_dist = is_sim ? SIM_SHOU_MEN_STAND_DIST : REAL_SHOU_MEN_STAND_DIST;
	const float shou_men_smooth = is_sim ? SIM_SHOU_MEN_SMOOTH : REAL_SHOU_MEN_SMOOTH;


	/*====================【功能块 2.2：读取场上信息】====================*/
	// 获取守门员当前位置
	const point2f& golie_pos = model->get_our_player_pos(robot_id);

	// 获取当前球的位置
	const point2f& ball = model->get_ball_pos();

	// 获取守门员当前车头朝向
	const float dir = model->get_our_player_dir(robot_id);

	// 获取我方球门中心点
	const point2f& goal = FieldPoint::Goal_Center_Point;

	// 判断球是否在我方禁区内
	bool ball_inside_penalty = is_inside_penalty(ball);


	/*====================【功能块 3：球在禁区内，守门员主动处理球】====================*/
	if (ball_inside_penalty)
	{
		/*====================【功能块 3.1：球在控球嘴附近，挑球解围】====================*/
		/*
		条件 1：
		球离守门员很近。

		条件 2：
		守门员车头基本对准球。

		满足后：
		执行挑球解围。
		*/
		float ball_dist = (ball - golie_pos).length();
		float ball_dir = (ball - golie_pos).angle();
		float dir_error = fabs(anglemod(dir - ball_dir));

		if (ball_dist < BALL_SIZE / 2 + MAX_ROBOT_SIZE + clear_ball_dist_extra &&
			dir_error < clear_angle_threshold)
		{
			task.kickPower = clear_kick_power;
			task.needKick = true;
			task.isChipKick = true;
		}

		/*====================【功能块 3.2：禁区内靠近球防守】====================*/
		/*
		球在禁区内时，守门员主动靠近球。

		task.orientate：
		让守门员车头朝向球。

		task.target_pos：
		让守门员移动到球附近，准备封堵或挑球。
		*/
		task.orientate = (ball - golie_pos).angle();
		task.target_pos = ball + Maths::vector2polar(BALL_SIZE / 2 + MAX_ROBOT_SIZE + active_target_extra, task.orientate);
	}


	/*====================【功能块 4：球不在禁区内，守门员站位防守】====================*/
	else
	{
		/*
		球不在禁区内时，守门员不主动冲出去。

		它会站在球门前 30 的位置，
		并根据球的位置左右移动，挡住球到球门的路线。
		*/
		task.orientate = (ball - goal).angle();
		task.target_pos = goal + Maths::vector2polar(stand_dist, task.orientate);
	}


	/*====================【功能块 5：守门员迟钝处理】====================*/
	/*
	这个功能块吸收之前 smooth 的逻辑。

	目的：
	让守门员不要每一帧都立刻跟着球变位置，
	而是慢慢靠近新的防守点。

	smooth 越大，越迟钝：
	0.75f：轻微迟钝
	0.85f：比较稳
	0.90f：明显迟钝
	0.95f：非常迟钝
	*/
	static bool has_last_task = false;
	static point2f last_target_pos;
	static float last_orientate = 0.0f;

	const float smooth = shou_men_smooth;

	if (!has_last_task)
	{
		last_target_pos = task.target_pos;
		last_orientate = task.orientate;
		has_last_task = true;
	}
	else
	{
		// 目标点迟钝处理：旧目标点占 smooth，新目标点占 1 - smooth
		last_target_pos.x = last_target_pos.x * smooth + task.target_pos.x * (1.0f - smooth);
		last_target_pos.y = last_target_pos.y * smooth + task.target_pos.y * (1.0f - smooth);

		// 朝向迟钝处理：用 anglemod 防止 PI 和 -PI 附近角度跳变
		float orientate_error = anglemod(task.orientate - last_orientate);
		last_orientate = anglemod(last_orientate + orientate_error * (1.0f - smooth));

		task.target_pos = last_target_pos;
		task.orientate = last_orientate;
	}


	/*====================【功能块 6：返回任务】====================*/
	return task;
}
#endif
