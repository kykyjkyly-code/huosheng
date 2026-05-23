#if 1
#include"src\utils\PlayerTask.h"
#include"src\getballsource.h"
#include"src\utils\worldmodel.h"
#include"src\utils\maths.h"
#include <cmath>

/*
功能流程：
1. 本文件负责传球：当前小车先靠近球后方，控住球后即刻推给接球队友。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 里会自动选择对应参数。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 传球调参区 ====================*/
// 没控住球时，站在球后方的距离
const float REAL_PASS_BEHIND_BALL_DIST = 13.0f;
const float SIM_PASS_BEHIND_BALL_DIST = 13.0f;
// 没控住球时，站在球后方的近距离
const float REAL_PASS_BEHIND_BALL_NEAR = 5.0f;
const float SIM_PASS_BEHIND_BALL_NEAR = 8.0f;
// 控球判断的角度阈值
const float REAL_PASS_ANGLE_THRESHOLD = 0.25f;
const float SIM_PASS_ANGLE_THRESHOLD = 0.25f;
// 踢球时车往前顶的微小距离
const float REAL_PASS_KICK_NUDGE_DIST = 2.0f;
const float SIM_PASS_KICK_NUDGE_DIST = 3.0f;
// 传球力度
const float REAL_PASS_KICK_POWER = 35.0f;
const float SIM_PASS_KICK_POWER = 60.0f;


// 修改后的 isget 判断，支持动态调整角度阈值
bool isget_dynamic(const WorldModel* model, int robot_id, float angle_threshold)
{
	// 获取球员位置
	const point2f& player_pos = model->get_our_player_pos(robot_id);

	// 获取球的位置
	const point2f& ball_pos = model->get_ball_pos();

	// 获取球员方向
	const float my_dir = model->get_our_player_dir(robot_id);

	// 小车到球的向量
	const point2f player_to_ball = ball_pos - player_pos;

	// 小车到球的距离
	const float ball_dist = player_to_ball.length();

	// 小车指向球的方向
	const float ball_dir = player_to_ball.angle();

	// 车头方向与球的方向差
	const float dir_error = fabs(ball_dir - my_dir);

	// 判断球是否离小车足够近
	const bool ball_near = ball_dist < get_ball_threshold;

	// 使用动态调整的角度阈值，已控球时放宽要求
	const bool ball_in_front = dir_error < angle_threshold;

	return ball_near && ball_in_front;
}


PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	PlayerTask task;

	if (model == NULL) {
		return task;
	}

	const bool is_sim = model->get_simulation();
	const float behind_ball_dist = is_sim ? SIM_PASS_BEHIND_BALL_DIST : REAL_PASS_BEHIND_BALL_DIST;
	const float behind_ball_near = is_sim ? SIM_PASS_BEHIND_BALL_NEAR : REAL_PASS_BEHIND_BALL_NEAR;
	const float angle_threshold = is_sim ? SIM_PASS_ANGLE_THRESHOLD : REAL_PASS_ANGLE_THRESHOLD;
	const float kick_nudge_dist = is_sim ? SIM_PASS_KICK_NUDGE_DIST : REAL_PASS_KICK_NUDGE_DIST;
	const float kick_power = is_sim ? SIM_PASS_KICK_POWER : REAL_PASS_KICK_POWER;

	// 找一个接球队员
	int receiver_id = -1;

	for (int i = 0; i < 6; i++)
	{
		// 不选自己也不选守门员
		if (i == robot_id || i == model->get_our_goalie())
			continue;

		// 找到一个存在的我方球员
		if (model->get_our_exist_id()[i])
		{
			receiver_id = i;
			break;
		}
	}

	// 如果没找到接球队员，先不处理
	if (receiver_id == -1)
	{
		return task;
	}

	// 获取当前球员位置
	const point2f& player_pos = model->get_our_player_pos(robot_id);

	// 获取球的位置
	const point2f& ball_pos = model->get_ball_pos();

	// 获取接球队员的位置
	const point2f& receiver_pos = model->get_our_player_pos(receiver_id);

	// 方向指向接球队员
	float face_dir = (receiver_pos - ball_pos).angle();

	// 默认：去球的后方，车头朝向接球队员
	task.orientate = face_dir;
	task.target_pos = ball_pos - Maths::vector2polar(behind_ball_dist, face_dir);

	// 默认开启吸球
	task.needCb = true;

	// 默认不踢球
	task.needKick = false;
	task.isPass = false;


	/*==================== 控球判断与传球触发 ====================*/
	if (isget_dynamic(model, robot_id, angle_threshold))
	{
		// 控住球了，车往前顶把球送出去
		task.target_pos = ball_pos + Maths::vector2polar(kick_nudge_dist, face_dir);
		task.orientate = face_dir;

		task.needCb = true;
		task.isChipKick = false;
		task.needKick = true;
		task.isPass = true;
		task.kickPower = kick_power;
	}
	else
	{
		// 还没控住球，站在球后方准备
		task.orientate = face_dir;
		task.target_pos = ball_pos - Maths::vector2polar(behind_ball_near, face_dir);

		task.needCb = true;
		task.needKick = false;
		task.isPass = false;
	}

	return task;
}
#endif
