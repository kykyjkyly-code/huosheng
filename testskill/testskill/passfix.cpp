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
const float SIM_PASS_BEHIND_BALL_NEAR = 6.0f;
// 控球判断的角度阈值
const float REAL_PASS_ANGLE_THRESHOLD = 0.05f;
const float SIM_PASS_ANGLE_THRESHOLD = 0.25f;
// 踢球时车往前顶的微小距离
const float REAL_PASS_KICK_NUDGE_DIST = 2.0f;
const float SIM_PASS_KICK_NUDGE_DIST = 3.0f;

// 传球力度
const float REAL_PASS_KICK_POWER = 25.0f;
const float SIM_PASS_KICK_POWER = 127.0f;


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

	// 车头方向与球的方向差（归一化后再取绝对值）
	const float dir_error = fabs(Maths::normalizeAngle(ball_dir - my_dir));

	// 判断球是否离小车足够近
	const bool ball_near = ball_dist < get_ball_threshold-1.0f;

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

	// 如果没找到接球队员，面向球并停在球后方等待
	if (receiver_id == -1)
	{
		const point2f& ball_pos = model->get_ball_pos();
		const point2f& player_pos = model->get_our_player_pos(robot_id);
		float faceBallDir = (ball_pos - player_pos).angle();
		task.orientate = faceBallDir;
		task.target_pos = ball_pos - Maths::vector2polar(MAX_ROBOT_SIZE + 10.0f, faceBallDir);
		task.needCb = true;
		return task;
	}

	// 获取当前球员位置
	const point2f& player_pos = model->get_our_player_pos(robot_id);

	// 获取球的位置
	const point2f& ball_pos = model->get_ball_pos();

	// 获取接球队员的位置和方向
	const point2f& receiver_pos = model->get_our_player_pos(receiver_id);
	const float receiver_dir = model->get_our_player_dir(receiver_id);
	// 接球队员控球嘴位置
	const point2f receiver_head = receiver_pos + Maths::vector2polar(static_cast<float>(ROBOT_HEAD), receiver_dir);

	// 方向：球到接球队员控球嘴
	float face_dir = (receiver_head - ball_pos).angle();

	/*==================== 绕球路径计算 ====================*/
	/*
	传球车不在正确球后方时，不能直接走向最终球后点，否则直线路径可能穿过球。
	这里把球看成圆心，让机器人每帧沿圆周最多转过一小段角度，逐步绕到
	接球队员反方向的球后位置；到达球后扇区后才允许直线贴近球。
	*/
	const float behind_dir = Maths::normalizeAngle(face_dir + static_cast<float>(PI));
	const point2f ball_to_player = player_pos - ball_pos;
	const float player_around_dir = ball_to_player.angle();
	const float behind_angle_error = Maths::normalizeAngle(behind_dir - player_around_dir);

	// 进入这个角度范围后，认为机器人已经位于正确球后侧。
	const float BEHIND_ANGLE_TOLERANCE = 0.35f;
	// 每帧绕球目标最多前进约 40 度，避免目标点突然跳到球的另一侧。
	const float MAX_ORBIT_ANGLE_STEP = 0.70f;
	const bool behind_ready = fabs(behind_angle_error) < BEHIND_ANGLE_TOLERANCE;

	float orbit_angle_step = behind_angle_error;
	if (orbit_angle_step > MAX_ORBIT_ANGLE_STEP)
	{
		orbit_angle_step = MAX_ORBIT_ANGLE_STEP;
	}
	else if (orbit_angle_step < -MAX_ORBIT_ANGLE_STEP)
	{
		orbit_angle_step = -MAX_ORBIT_ANGLE_STEP;
	}

	const float orbit_dir = Maths::normalizeAngle(player_around_dir + orbit_angle_step);
	const point2f orbit_target = ball_pos
		+ Maths::vector2polar(behind_ball_dist, orbit_dir);

	// 近距离参数不能小于机器人和球的几何安全距离，否则目标点会落进球内。
	const float min_safe_behind_dist = static_cast<float>(
		MAX_ROBOT_SIZE + BALL_SIZE / 2 + 1.0f
	);
	const float final_behind_dist = behind_ball_near > min_safe_behind_dist
		? behind_ball_near : min_safe_behind_dist;

	// 默认：去球的后方，车头朝向接球队员
	task.orientate = face_dir;
	task.target_pos = ball_pos - Maths::vector2polar(behind_ball_dist, face_dir);

	// 默认开启吸球
	task.needCb = true;

	// 默认不踢球
	task.needKick = false;
	task.isPass = false;


	/*==================== 控球判断与传球触发 ====================*/
	if (behind_ready && isget_dynamic(model, robot_id, angle_threshold))
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
		if (!behind_ready)
		{
			// 还未到正确球后侧：沿球外圈分段绕行，车头始终朝向球。
			task.orientate = (ball_pos - player_pos).angle();
			task.target_pos = orbit_target;
			// 绕行阶段关闭吸球，避免从侧面提前带走小球。
			task.needCb = false;
		}
		else
		{
			// 已经位于球后侧：沿传球方向贴近球并打开吸球。
			task.orientate = face_dir;
			task.target_pos = ball_pos
				- Maths::vector2polar(final_behind_dist, face_dir);
			task.needCb = true;
		}

		task.needKick = false;
		task.isPass = false;
	}

	return task;
}
#endif
