#if 0
#include "src\utils\PlayerTask.h"
#include "src\getballsource.h"
#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"
#include <cmath>

/*
功能流程：
1. 本文件负责观察对方守门员移动趋势，等空档打开后平射。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 和辅助判断会自动选择对应参数。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 射门调参区 ====================*/
/*
	实际调试时优先改这里，不用到 player_plan 里面找数字。

	坐标说明：
	y < 0 通常表示球门左侧，y > 0 通常表示球门右侧。
	本策略假设对方守门员从左向右移动时，左侧会出现空档。
*/

// 小车控到球时，球必须在车头方向的角度误差范围内，单位是弧度。
const float REAL_SHOOT_MOUTH_ANGLE_THRESHOLD = 0.08f;
const float SIM_SHOOT_MOUTH_ANGLE_THRESHOLD = 0.08f;

// 拿球时站在球后方的距离。越大越保守，越小越贴球。
const float REAL_SHOOT_GET_BALL_BACK_DIST = 13.0f;
const float SIM_SHOOT_GET_BALL_BACK_DIST = 13.0f;

// 如果守门员没有被映射出来，就在对方球门附近扫描对方球员。
const float REAL_GOALIE_SEARCH_X_RANGE = 70.0f;
const float REAL_GOALIE_SEARCH_Y_RANGE = 80.0f;
const float SIM_GOALIE_SEARCH_X_RANGE = 70.0f;
const float SIM_GOALIE_SEARCH_Y_RANGE = 80.0f;

// 扫描守门员时，y 方向距离的权重。越大越重视守门员是否贴近球门中心。
const float REAL_GOALIE_SEARCH_Y_WEIGHT = 0.2f;
const float SIM_GOALIE_SEARCH_Y_WEIGHT = 0.2f;

// 判断守门员“正在向右移动”的最小 y 方向变化量。
const float REAL_GOALIE_MOVE_RIGHT_THRESHOLD = 0.4f;
const float SIM_GOALIE_MOVE_RIGHT_THRESHOLD = 0.4f;

// 守门员连续向右移动多少帧后，才认为趋势稳定。
const int REAL_GOALIE_STABLE_MOVE_FRAMES = 3;
const int SIM_GOALIE_STABLE_MOVE_FRAMES = 3;

// 守门员离开球门中心多少距离后，才认为左侧空档打开。
const float REAL_GOALIE_LEAVE_CENTER_Y = 6.0f;
const float SIM_GOALIE_LEAVE_CENTER_Y = 6.0f;

// 射门目标距离门柱的安全边距，避免瞄太偏打到门柱外。
const float REAL_GOAL_SHOOT_MARGIN = 8.0f;
const float SIM_GOAL_SHOOT_MARGIN = 8.0f;

// 平射力度。太小容易被守门员挡住，太大可能控制不稳。
const double REAL_SHOOT_KICK_POWER = 35.0;
const double SIM_SHOOT_KICK_POWER = 35.0;

// 控球后最多等多少帧；超过后不再死等守门员，直接打当前空档。
const int REAL_SHOOT_MAX_WAIT_FRAMES = 90;
const int SIM_SHOOT_MAX_WAIT_FRAMES = 90;

float normalizeAngle(float angle)
{
	while (angle > PI) angle -= 2 * PI;
	while (angle < -PI) angle += 2 * PI;
	return angle;
}

bool isget(const WorldModel* model, int robot_id)
{
	if (model == NULL) {
		return false;
	}

	if (robot_id < 0 || robot_id >= 6) {
		return false;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float mouth_angle_threshold = is_sim ? SIM_SHOOT_MOUTH_ANGLE_THRESHOLD : REAL_SHOOT_MOUTH_ANGLE_THRESHOLD;

	const point2f& player_pos = model->get_our_player_pos(robot_id);
	const point2f& ball_pos = model->get_ball_pos();
	const float my_dir = model->get_our_player_dir(robot_id);

	const point2f player_to_ball = ball_pos - player_pos;
	const float ball_dist = player_to_ball.length();
	const float ball_dir = player_to_ball.angle();
	const float dir_error = static_cast<float>(fabs(normalizeAngle(ball_dir - my_dir)));

	// 同时满足“距离近”和“球在车头前方”，才认为已经控到球。
	const bool ball_near = ball_dist < get_ball_threshold;
	const bool ball_in_front = dir_error < mouth_angle_threshold;

	return ball_near && ball_in_front;
}

float clampFloat(float value, float min_value, float max_value)
{
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

bool findOpponentGoalie(const WorldModel* model, const point2f& opp_goal, point2f& goalie_pos)
{
	if (model == NULL) {
		return false;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float goalie_search_x_range = is_sim ? SIM_GOALIE_SEARCH_X_RANGE : REAL_GOALIE_SEARCH_X_RANGE;
	const float goalie_search_y_range = is_sim ? SIM_GOALIE_SEARCH_Y_RANGE : REAL_GOALIE_SEARCH_Y_RANGE;
	const float goalie_search_y_weight = is_sim ? SIM_GOALIE_SEARCH_Y_WEIGHT : REAL_GOALIE_SEARCH_Y_WEIGHT;

	const bool* opp_exist = model->get_opp_exist_id();
	const int opp_goalie_id = model->get_opp_goalie();

	if (opp_goalie_id >= 0 && opp_goalie_id < 6) {
		if (opp_exist == NULL || opp_exist[opp_goalie_id]) {
			goalie_pos = model->get_opp_player_pos(opp_goalie_id);
			return true;
		}
	}

	bool found = false;
	float best_score = 1000000.0f;

	for (int i = 0; i < 6; i++) {
		if (opp_exist != NULL && !opp_exist[i]) {
			continue;
		}

		const point2f& pos = model->get_opp_player_pos(i);
		const float dx = static_cast<float>(fabs(pos.x - opp_goal.x));
		const float dy = static_cast<float>(fabs(pos.y - opp_goal.y));

		if (dx > goalie_search_x_range || dy > goalie_search_y_range) {
			continue;
		}

		const float score = dx + dy * goalie_search_y_weight;
		if (!found || score < best_score) {
			found = true;
			best_score = score;
			goalie_pos = pos;
		}
	}

	return found;
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

	const bool is_sim = model != NULL && model->get_simulation();
	const float get_ball_back_dist = is_sim ? SIM_SHOOT_GET_BALL_BACK_DIST : REAL_SHOOT_GET_BALL_BACK_DIST;
	const float goalie_move_right_threshold = is_sim ? SIM_GOALIE_MOVE_RIGHT_THRESHOLD : REAL_GOALIE_MOVE_RIGHT_THRESHOLD;
	const int goalie_stable_move_frames = is_sim ? SIM_GOALIE_STABLE_MOVE_FRAMES : REAL_GOALIE_STABLE_MOVE_FRAMES;
	const float goalie_leave_center_y = is_sim ? SIM_GOALIE_LEAVE_CENTER_Y : REAL_GOALIE_LEAVE_CENTER_Y;
	const float goal_shoot_margin = is_sim ? SIM_GOAL_SHOOT_MARGIN : REAL_GOAL_SHOOT_MARGIN;
	const double shoot_kick_power = is_sim ? SIM_SHOOT_KICK_POWER : REAL_SHOOT_KICK_POWER;
	const int shoot_max_wait_frames = is_sim ? SIM_SHOOT_MAX_WAIT_FRAMES : REAL_SHOOT_MAX_WAIT_FRAMES;

	const point2f& player_pos = model->get_our_player_pos(robot_id);
	const point2f& ball_pos = model->get_ball_pos();

	// 对方球门中心在右侧，所以直接用 FIELD_LENGTH_H。
	point2f opp_goal(static_cast<float>(FIELD_LENGTH_H), 0.0f);
	point2f shoot_target = opp_goal;

	// 读取对方守门员位置；如果系统没有映射守门员，就扫描对方球门附近的对方球员。
	point2f opp_goalie_pos = opp_goal;
	bool has_goalie = findOpponentGoalie(model, opp_goal, opp_goalie_pos);

	/*==================== 守门员运动检测 ====================*/
	/*
		检测思路：
		1. 记录上一帧守门员的 y 坐标。
		2. 当前帧 y 坐标变大，说明守门员正在从左往右移动。
		3. 连续多帧向右移动后，再认为守门员移动趋势稳定。
	*/
	static bool has_last_goalie_y = false;
	static float last_goalie_y = 0.0f;
	static int right_move_frames = 0;
	static int hold_ball_frames[6] = { 0 };

	const float goalie_y = opp_goalie_pos.y;
	const float goalie_delta_y = has_last_goalie_y ? goalie_y - last_goalie_y : 0.0f;

	if (has_goalie && has_last_goalie_y && goalie_delta_y > goalie_move_right_threshold) {
		right_move_frames++;
	}
	else if (has_goalie && has_last_goalie_y && goalie_delta_y < -goalie_move_right_threshold) {
		right_move_frames = 0;
	}

	last_goalie_y = goalie_y;
	has_last_goalie_y = has_goalie;

	// 球门上下两个可瞄准点，留出 GOAL_SHOOT_MARGIN，避免瞄得太贴门柱。
	const float goal_low_y = static_cast<float>(-GOAL_WIDTH_H + goal_shoot_margin);
	const float goal_high_y = static_cast<float>(GOAL_WIDTH_H - goal_shoot_margin);

	// 守门员向右离开中心后，左侧空档打开，可以射左下角。
	bool goalie_moving_left_to_right = right_move_frames >= goalie_stable_move_frames;
	bool goalie_has_opened_low_side = has_goalie && goalie_moving_left_to_right && goalie_y > goalie_leave_center_y;
	bool can_shoot_now = goalie_has_opened_low_side;
	bool wait_too_long = false;

	if (isget(model, robot_id)) {
		hold_ball_frames[robot_id]++;
		wait_too_long = hold_ball_frames[robot_id] >= shoot_max_wait_frames;
	}
	else {
		hold_ball_frames[robot_id] = 0;
	}

	if (can_shoot_now) {
		shoot_target.y = goal_low_y;
	}
	else if (has_goalie) {
		// 还不能射时，先瞄准守门员所在位置的反方向，准备等空档。
		float opposite_y = goalie_y > 0.0f ? goal_low_y : goal_high_y;
		shoot_target.y = clampFloat(opposite_y, goal_low_y, goal_high_y);
	}

	const float face_dir = (shoot_target - ball_pos).angle();

	// 默认动作：移动到球后方、车头朝向射门点、打开吸球，不立即射门。
	task.orientate = face_dir;
	task.target_pos = ball_pos - Maths::vector2polar(get_ball_back_dist, face_dir);
	task.needCb = true;
	task.needKick = false;
	task.isPass = false;
	task.isChipKick = false;

	if (isget(model, robot_id)) {
		// 已经控到球后，停在当前位置等待射门时机。
		task.target_pos = player_pos;
		task.orientate = face_dir;
		task.needCb = true;

		if (can_shoot_now || wait_too_long || !has_goalie) {
			// 守门员露出空档、等待超时，或者没有识别到守门员，就执行平射。
			task.needKick = true;
			task.isPass = false;
			task.isChipKick = false;
			task.kickPower = shoot_kick_power;
		}
	}

	return task;
}
#endif

