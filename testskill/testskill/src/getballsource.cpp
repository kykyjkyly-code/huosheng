#include "getballsource.h"

#include <cmath>
#include <vector>

#include "utils\ball.h"
#include "utils\constants.h"
#include "utils\maths.h"
#include "utils\worldmodel.h"

namespace {
/*
功能流程：
1. 本文件提供通用拿球 GetBall：判断球是否运动，选择直线拿球或绕球拿球。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，plan 里会自动选择对应参数。
*/

/*==================== GetBall 调参区 ====================*/
// 实车拿球缓冲距离。
const double REAL_GET_BALL_BUF = 5.0;
// 仿真拿球缓冲距离。
const double SIM_GET_BALL_BUF = -4.0;
// 实车绕开球时后撤距离。
const float REAL_AWAY_BALL_DIST_X = 40.0f;
// 仿真绕开球时后撤距离。
const float SIM_AWAY_BALL_DIST_X = 20.0f;
// 判断球在运动的最小位移。
const float REAL_BALL_MOVE_THRESHOLD = 0.8f;
const float SIM_BALL_MOVE_THRESHOLD = 0.8f;
// 小车在球前方时侧向绕开的距离。
const float REAL_SIDE_STEP_DIST = 35.0f;
const float SIM_SIDE_STEP_DIST = 35.0f;
// 控球嘴距离判断。
const float REAL_CLOSE_BALL_DIST = get_ball_threshold;
const float SIM_CLOSE_BALL_DIST = get_ball_threshold;
// 控球嘴角度判断。
const float REAL_MOUTH_ANGLE = static_cast<float>(PI / 6);
const float SIM_MOUTH_ANGLE = static_cast<float>(PI / 6);
// 计算球运动方向时使用的历史点数量。
const int REAL_BALL_HISTORY_POINTS = 8;
const int SIM_BALL_HISTORY_POINTS = 8;

double get_ball_buf = REAL_GET_BALL_BUF;
float away_ball_dist_x = REAL_AWAY_BALL_DIST_X;

float angle_diff(float a, float b)
{
	return static_cast<float>(anglemod(a - b));
}

point2f polar(float length, float dir)
{
	return Maths::vector2polar(length, dir);
}

point2f point_behind_ball(const point2f& ball, float aim_dir, float dist)
{
	return ball + polar(dist, static_cast<float>(anglemod(aim_dir + PI)));
}
}

GetBall::GetBall()
{
}

GetBall::~GetBall()
{
}

bool GetBall::toward_opp_goal(float dir)
{
	return dir < PI / 2 && dir > -PI / 2;
}

float GetBall::ball_x_angle(const WorldModel* model)
{
	if (model == NULL) {
		return 0.0f;
	}

	std::vector<point2f> ball_points;
	const bool is_sim = model != NULL && model->get_simulation();
	const int ball_history_points = is_sim ? SIM_BALL_HISTORY_POINTS : REAL_BALL_HISTORY_POINTS;
	ball_points.reserve(ball_history_points);
	for (int i = 0; i < ball_history_points; i++) {
		ball_points.push_back(model->get_ball_pos(i));
	}

	return Maths::least_squares(ball_points);
}

PlayerTask GetBall::plan(const WorldModel* model, int robot_id, int receiver_id)
{
	PlayerTask task;

	if (model == NULL) {
		return task;
	}

	if (model->get_simulation()) {
		get_ball_buf = SIM_GET_BALL_BUF;
		away_ball_dist_x = SIM_AWAY_BALL_DIST_X;
	}
	else {
		get_ball_buf = REAL_GET_BALL_BUF;
		away_ball_dist_x = REAL_AWAY_BALL_DIST_X;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float ball_move_threshold = is_sim ? SIM_BALL_MOVE_THRESHOLD : REAL_BALL_MOVE_THRESHOLD;
	const float side_step_dist = is_sim ? SIM_SIDE_STEP_DIST : REAL_SIDE_STEP_DIST;
	const float close_ball_dist = is_sim ? SIM_CLOSE_BALL_DIST : REAL_CLOSE_BALL_DIST;
	const float mouth_angle = is_sim ? SIM_MOUTH_ANGLE : REAL_MOUTH_ANGLE;

	const point2f& ball = model->get_ball_pos();
	const point2f& last_ball = model->get_ball_pos(1);
	const point2f& get_ball_player = model->get_our_player_pos(robot_id);
	const point2f& receive_ball_player = model->get_our_player_pos(receiver_id);
	const float dir = model->get_our_player_dir(robot_id);
	const float rece_dir = model->get_our_player_dir(receiver_id);

	const point2f opp_goal = -FieldPoint::Goal_Center_Point;
	const point2f rece_head_pos = receive_ball_player + polar(static_cast<float>(ROBOT_HEAD), rece_dir);
	const point2f aim_pos = receiver_id == robot_id ? opp_goal : rece_head_pos;
	const float aim_dir = (aim_pos - ball).angle();
	const float approach_dir = static_cast<float>(anglemod(aim_dir + PI));

	const point2f ball_delta = ball - last_ball;
	const float ball_moving_dist = ball_delta.length();
	point2f ball_with_vel = ball;
	if (ball_moving_dist >= ball_move_threshold) {
		ball_with_vel = ball + polar(ball_moving_dist, ball_delta.angle());
	}

	const point2f player_to_ball = ball - get_ball_player;
	const float player_away_ball = player_to_ball.length();
	const float mouth_angle_err = std::fabs(angle_diff(player_to_ball.angle(), dir));
	const bool ball_in_mouth = player_away_ball < close_ball_dist && mouth_angle_err < mouth_angle;

	const float hold_dist = static_cast<float>(ROBOT_HEAD + BALL_SIZE / 2);
	const float get_ball_dist = static_cast<float>(BALL_SIZE / 2 + MAX_ROBOT_SIZE + get_ball_buf);

	task.orientate = aim_dir;

	if (ball_in_mouth) {
		task.target_pos = point_behind_ball(ball, aim_dir, hold_dist);
		return task;
	}

	const point2f ball_to_player = get_ball_player - ball;
	const point2f aim_vec = polar(1.0f, aim_dir);
	const bool player_in_front_of_ball = dot(ball_to_player, aim_vec) > 0.0f;

	if (player_in_front_of_ball) {
		const float cross_value = perp(aim_vec, ball_to_player);
		const float side_dir = approach_dir + (cross_value >= 0.0f ? -static_cast<float>(PI / 2) : static_cast<float>(PI / 2));
		task.target_pos = ball_with_vel + polar(away_ball_dist_x, approach_dir) + polar(side_step_dist, side_dir);
	}
	else {
		task.target_pos = point_behind_ball(ball_with_vel, aim_dir, get_ball_dist);
	}

	return task;
}
