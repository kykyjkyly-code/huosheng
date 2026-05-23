#if 1
#include "src\utils\PlayerTask.h"
#include "src\getballsource.h"
#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"
#include <cmath>

/*
功能流程：
1. 本文件负责双人传球协调：当前小车与一名队友配合，
   根据与对方球门的距离自动分配"传球者"和"接球者"角色。
2. 离对方球门更近的为接球者（Receiver），更远的为传球者（Passer）。
3. 传球者：绕到球后方，对准接球者推球。
4. 接球者：移动到传球路线上的接收点，面向来球准备接球。
5. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 调参区 ====================*/
// 传球者绕球半径额外距离
const float REAL_PASS_CIRCLE_EXTRA = 12.0f;
const float SIM_PASS_CIRCLE_EXTRA = 12.0f;
// 传球者到达绕球位置的容差
const float REAL_PASS_ARRIVE_ERR = 5.0f;
const float SIM_PASS_ARRIVE_ERR = 5.0f;
// 传球者角度对齐阈值：根据传接距离动态调整
// 距离远 → 粗略角度，距离近 → 精确角度
const float REAL_PASS_ALIGN_ANGLE_FAR = 0.30f;    // 远距离时的粗略角度（弧度）
const float SIM_PASS_ALIGN_ANGLE_FAR = 0.30f;
const float REAL_PASS_ALIGN_ANGLE_NEAR = 0.08f;   // 近距离时的精确角度（弧度）
const float SIM_PASS_ALIGN_ANGLE_NEAR = 0.08f;
const float REAL_PASS_ALIGN_DIST_THRESH = 80.0f;  // 远近分界距离
const float SIM_PASS_ALIGN_DIST_THRESH = 80.0f;
// 接球者站在传球方向上的前方距离
const float REAL_RECEIVE_AHEAD_DIST = 60.0f;
const float SIM_RECEIVE_AHEAD_DIST = 60.0f;
// 传球距离阈值：传球者和接球者距离超过此值才传球
const float REAL_MIN_PASS_DIST = 30.0f;
const float SIM_MIN_PASS_DIST = 30.0f;


PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	/*==================== 功能块 1：初始化任务 ====================*/
	PlayerTask task;

	if (model == NULL) {
		return task;
	}

	const bool is_sim = model->get_simulation();
	const float pass_circle_extra = is_sim ? SIM_PASS_CIRCLE_EXTRA : REAL_PASS_CIRCLE_EXTRA;
	const float pass_arrive_err = is_sim ? SIM_PASS_ARRIVE_ERR : REAL_PASS_ARRIVE_ERR;
	const float pass_align_angle_far = is_sim ? SIM_PASS_ALIGN_ANGLE_FAR : REAL_PASS_ALIGN_ANGLE_FAR;
	const float pass_align_angle_near = is_sim ? SIM_PASS_ALIGN_ANGLE_NEAR : REAL_PASS_ALIGN_ANGLE_NEAR;
	const float pass_align_dist_thresh = is_sim ? SIM_PASS_ALIGN_DIST_THRESH : REAL_PASS_ALIGN_DIST_THRESH;
	const float receive_ahead_dist = is_sim ? SIM_RECEIVE_AHEAD_DIST : REAL_RECEIVE_AHEAD_DIST;
	const float min_pass_dist = is_sim ? SIM_MIN_PASS_DIST : REAL_MIN_PASS_DIST;

	/*==================== 功能块 2：寻找一名队友 ====================*/
	// 找一名非自己、非守门员的队友参与传球配合
	int teammate_id = -1;

	for (int i = 0; i < 6; i++)
	{
		if (i == robot_id || i == model->get_our_goalie())
			continue;

		if (model->get_our_exist_id()[i])
		{
			teammate_id = i;
			break;
		}
	}

	// 没有队友时，小车面向球并停在球后方等待
	if (teammate_id == -1)
	{
		const point2f& ball_pos = model->get_ball_pos();
		const point2f& player_pos = model->get_our_player_pos(robot_id);
		float faceBallDir = (ball_pos - player_pos).angle();
		task.orientate = faceBallDir;
		task.target_pos = ball_pos - Maths::vector2polar(MAX_ROBOT_SIZE + 10.0f, faceBallDir);
		task.needCb = true;
		return task;
	}

	/*==================== 功能块 3：获取位置信息 ====================*/
	const point2f& ball_pos = model->get_ball_pos();
	const point2f& player_pos = model->get_our_player_pos(robot_id);
	const float player_dir = model->get_our_player_dir(robot_id);

	const point2f& teammate_pos = model->get_our_player_pos(teammate_id);
	const float teammate_dir = model->get_our_player_dir(teammate_id);

	// 对方球门中心
	const point2f opp_goal = FieldPoint::Goal_Center_Point;

	/*==================== 功能块 4：分配角色 ====================*/
	// 离对方球门更近的是接球者（Receiver），更远的是传球者（Passer）
	float my_dist_to_goal = (player_pos - opp_goal).length();
	float teammate_dist_to_goal = (teammate_pos - opp_goal).length();

	bool i_am_receiver = (my_dist_to_goal < teammate_dist_to_goal);

	/*==================== 功能块 5：计算传球方向 ====================*/
	// 接球者车头（控球嘴）位置
	const point2f receiverHeadPos = i_am_receiver
		? (player_pos + Maths::vector2polar(static_cast<float>(ROBOT_HEAD), player_dir))
		: (teammate_pos + Maths::vector2polar(static_cast<float>(ROBOT_HEAD), teammate_dir));

	// 传球方向：球 → 接球者车头
	float passDir = (receiverHeadPos - ball_pos).angle();

	// 传、接球者之间的距离
	float passer_to_receiver_dist = i_am_receiver
		? (teammate_pos - player_pos).length()
		: (player_pos - teammate_pos).length();

	task.needCb = true;

	/*==================== 功能块 6：执行角色行为 ====================*/
	if (i_am_receiver)
	{
		/*==================== 我是接球者 ====================*/
		// 站在传球方向的前方，面向来球方向
		// 接球点 = 球 + 传球方向 * 前方距离
		point2f receive_target = ball_pos + Maths::vector2polar(receive_ahead_dist, passDir);

		task.target_pos = receive_target;
		// 面向球的方向（面对来球）
		task.orientate = (ball_pos - player_pos).angle();

		// 如果传球者和接球者距离够远，且球已经在传来，准备踢球
		if (passer_to_receiver_dist > min_pass_dist)
		{
			// 球离接球者很近时，可以踢球射门
			float dist_to_ball = (player_pos - ball_pos).length();
			if (dist_to_ball < MAX_ROBOT_SIZE + BALL_SIZE + 5.0f)
			{
				task.needKick = true;
				task.kickPower = 127;
				task.isChipKick = false;
				// 朝向对方球门踢
				task.orientate = (opp_goal - ball_pos).angle();
			}
		}
	}
	else
	{
		/*==================== 我是传球者 ====================*/
		// 绕到球的后方（球在传球者和接球者之间），对准接球者后推球

		// 球后方方向 = 传球方向的反方向
		float behindBallDir = Maths::normalizeAngle(passDir + static_cast<float>(PI));

		// 绕球半径
		float circleR = MAX_ROBOT_SIZE + pass_circle_extra;

		// 目标点：球后方，距离球 circleR
		point2f target_behind_ball = ball_pos + Maths::vector2polar(circleR, behindBallDir);

		task.target_pos = target_behind_ball;

		// 车头朝向球（绕球过程中一直面向球）
		task.orientate = (ball_pos - player_pos).angle();

		// 判断是否已经到达球后方并对准了接球者
		float dist_to_target = (player_pos - target_behind_ball).length();
		float current_angle_to_ball = (ball_pos - player_pos).angle();
		float angle_err = fabs(Maths::normalizeAngle(passDir - current_angle_to_ball));

		// 动态角度阈值：距离远 → 粗略，距离近 → 精确
		float dynamic_align_angle;
		if (passer_to_receiver_dist > pass_align_dist_thresh)
		{
			dynamic_align_angle = pass_align_angle_far;
		}
		else
		{
			// 在 [0, dist_thresh] 范围内线性插值：距离越小要求越精确
			float ratio = passer_to_receiver_dist / pass_align_dist_thresh;
			dynamic_align_angle = pass_align_angle_near + ratio * (pass_align_angle_far - pass_align_angle_near);
		}

		// 已经就位且角度对齐 → 推球传球
		if (dist_to_target < pass_arrive_err && angle_err < dynamic_align_angle)
		{
			// 朝向接球者
			task.orientate = passDir;

			if (passer_to_receiver_dist > min_pass_dist)
			{
				task.needKick = true;
				task.kickPower = 80;  // 传球力度适中
				task.isChipKick = false;
			}
		}
	}

	return task;
}
#endif