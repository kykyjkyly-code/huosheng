#if 1
#include "src\utils\PlayerTask.h"
#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"
#include <cmath>

/*
功能流程（测试用）：
1. 守门员默认站在 (-100, -50)，作为"控球嘴条件未触发"的视觉标记。
2. 每帧检测 2号车 的控球嘴附近是否有球：
   - 球到2号车的距离 < get_ball_threshold + offset
   - 球方向与2号车朝向的夹角 < mouth_angle
3. 条件满足 → 守门员移动到 (-100, -100)，表示"控球嘴条件已触发"。
4. 条件不满足 → 守门员回到 (-100, -50)。

约定：
- robot_id 是守门员自己的号码，本文件不拿它做传球逻辑。
- 2号 = 传球车（PASSER_ID=2），1号 = 接球车（RECEIVER_ID=1，暂未使用）。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 调参区 ====================*/

// 守门员默认位置（控球嘴条件未触发）
const float GOALIE_DEFAULT_X = -100.0f;
const float GOALIE_DEFAULT_Y = -50.0f;

// 守门员触发后位置（控球嘴条件已触发）
const float GOALIE_TRIGGERED_X = -100.0f;
const float GOALIE_TRIGGERED_Y = -100.0f;

// 2号传球车控球嘴判定 —— 距离偏移
const float REAL_PASSER_GET_BALL_DIST_OFFSET = 5.0f;
const float SIM_PASSER_GET_BALL_DIST_OFFSET = 5.0f;

// 2号传球车控球嘴判定 —— 角度阈值（弧度）
const float REAL_PASSER_MOUTH_ANGLE = 0.14f;
const float SIM_PASSER_MOUTH_ANGLE = 0.18f;

// 固定传球目标点（1号接球车应去的位置）
const float REAL_PASS_TARGET_X = 20.0f;
const float REAL_PASS_TARGET_Y = 70.0f;
const float SIM_PASS_TARGET_X = 20.0f;
const float SIM_PASS_TARGET_Y = 70.0f;

// 车号配置
const int PASSER_ID = 2;    // 2号传球车
const int RECEIVER_ID = 1;  // 1号接球车（暂未使用）


PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	PlayerTask task;

	if (model == NULL) {
		return task;
	}

	if (robot_id < 0 || robot_id >= 6) {
		return task;
	}

	const bool is_sim = model->get_simulation();
	const float passer_get_ball_dist_offset = is_sim
		? SIM_PASSER_GET_BALL_DIST_OFFSET : REAL_PASSER_GET_BALL_DIST_OFFSET;
	const float passer_mouth_angle = is_sim
		? SIM_PASSER_MOUTH_ANGLE : REAL_PASSER_MOUTH_ANGLE;
	const float pass_target_x = is_sim ? SIM_PASS_TARGET_X : REAL_PASS_TARGET_X;
	const float pass_target_y = is_sim ? SIM_PASS_TARGET_Y : REAL_PASS_TARGET_Y;

	const point2f& ball_pos = model->get_ball_pos();
	const point2f pass_target(pass_target_x, pass_target_y);

	// ================================================================
	//  检测 2号车 控球嘴附近是否有球
	// ================================================================

	/*
	 *  向量:  passer_to_ball = ball_pos - passer_pos
	 *  距离:  passer_ball_dist = |passer_to_ball|
	 *  方向:  passer_ball_dir  = (pass_target - ball_pos).angle()
	 *         （球 → 目标点(20,70) 的方向，即传球方向）
	 *
	 *  条件A: passer_ball_dist < get_ball_threshold + offset
	 *  条件B: |passer_ball_dir - passer_dir| < mouth_angle
	 */
	const point2f& passer_pos = model->get_our_player_pos(PASSER_ID);
	const point2f& receiver_pos = model->get_our_player_pos(RECEIVER_ID);
	const point2f passer_to_ball = ball_pos - passer_pos;
	const float passer_ball_dist = passer_to_ball.length();
	const point2f ball_to_target = pass_target - ball_pos;
	const float passer_ball_dir = ball_to_target.angle();

	const float passer_dir = model->get_our_player_dir(PASSER_ID);
	const float passer_dir_error = static_cast<float>(
		fabs(Maths::normalizeAngle(passer_ball_dir - passer_dir))
	);

	const bool ball_near = passer_ball_dist < get_ball_threshold + passer_get_ball_dist_offset;
	const bool ball_in_front = passer_dir_error < passer_mouth_angle;
	const bool passer_has_ball = ball_near && ball_in_front;

	// ================================================================
	//  守门员位置：根据2号控球嘴条件切换
	// ================================================================

	if (passer_has_ball) {
		// 条件触发 → 守门员移到 (-100, -100)
		task.target_pos = point2f(GOALIE_TRIGGERED_X, GOALIE_TRIGGERED_Y);
	}
	else {
		// 条件未触发 → 守门员在 (-100, -50)
		task.target_pos = point2f(GOALIE_DEFAULT_X, GOALIE_DEFAULT_Y);
	}

	// 始终面向球
	task.orientate = (ball_pos - task.target_pos).angle();
	task.needCb = true;
	task.needKick = false;

	return task;
}
#endif
