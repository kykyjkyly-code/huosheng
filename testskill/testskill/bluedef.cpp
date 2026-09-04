#if 1

#include "utils\PlayerTask.h"
#include "getballsource.h"
#include "utils\worldmodel.h"
#include "utils\maths.h"
#include <cmath>

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	PlayerTask task;

	if (model == NULL) {
		return task;
	}

	// 1. 获取基本位置信息
	const point2f& player_pos = model->get_our_player_pos(robot_id);
	const point2f& ball_pos = model->get_ball_pos();
	point2f our_goal_center = FieldPoint::Goal_Center_Point;

	// 获取上一帧球的位置，用来计算球速，判断是否处于传接球失误的自由滚动状态
	const point2f& last_ball_pos = model->get_ball_pos(1);
	float ball_speed = (ball_pos - last_ball_pos).length();

	// 2. 检测对方谁离球最近，以及最近的距离是多少
	float min_opp_to_ball_dist = 9999.0f;
	for (int i = 0; i < 6; i++)
	{
		if (model->get_opp_exist_id()[i])
		{
			float dist = (model->get_opp_player_pos(i) - ball_pos).length();
			if (dist < min_opp_to_ball_dist)
			{
				min_opp_to_ball_dist = dist;
			}
		}
	}

	// 3. 核心决策：对方是否传接球失误？
	// 判断标准：球在快速滚动(说明在传球)，且对方最近的车离球有一定距离(说明还没接稳)
	// 或者对方所有人离球都极远（大失误）
	bool opp_mistake = (ball_speed > 2.0f && min_opp_to_ball_dist > 40.0f) || (min_opp_to_ball_dist > 60.0f);

	// 计算基础卡位方向（球指向我方球门）
	float ball_to_goal_dir = (our_goal_center - ball_pos).angle();
	point2f raw_target_pos;

	if (opp_mistake)
	{
		// 【捡漏本能】：对方失误了！解除安全距离，直接向球冲过去
		raw_target_pos = ball_pos;
		task.needCb = true; // 打开吸球准备断球
	}
	else
	{
		// 【慢速跟球】：对方控球很稳。保持呆呆的社交距离（35.0f），不上去硬抢
		const float DEFEND_KEEP_DIST = 35.0f;
		raw_target_pos = ball_pos + Maths::vector2polar(DEFEND_KEEP_DIST, ball_to_goal_dir);
		task.needCb = false; // 正常跟随不需要开吸球
	}

	// 4. 减缓车速（适度迟缓感逻辑）
	// 不要让小车一步到位，这一帧的目标点只向预定目标点挪动 25% 的距离
	// 这样小车跑起来会显得柔和、温和，有一种慢半拍的优雅，但又不至于太慢
	float speed_reducer = 0.25f;
	point2f smooth_target_pos = player_pos + (raw_target_pos - player_pos) * speed_reducer;

	// 5. 强力防撞机制
	// 遍历对方场上的所有机器人，如果发现离我的平滑目标点或者我本人太近，强制后退避让
	const float AVOID_DIST = MAX_ROBOT_SIZE * 2.5f;
	bool need_avoid = false;

	for (int i = 0; i < 6; i++)
	{
		if (model->get_opp_exist_id()[i])
		{
			const point2f& opp_pos = model->get_opp_player_pos(i);
			float dist_to_me = (player_pos - opp_pos).length();
			float dist_to_target = (smooth_target_pos - opp_pos).length();

			// 如果对方快要撞到我，或者快要撞到我的目的地
			if (dist_to_me < AVOID_DIST || dist_to_target < AVOID_DIST)
			{
				need_avoid = true;
				break;
			}
		}
	}

	// 执行避让：如果危险，目标点强行向我方球门后撤，把位置拱手让给对方
	if (need_avoid)
	{
		smooth_target_pos = smooth_target_pos + Maths::vector2polar(AVOID_DIST, ball_to_goal_dir);
		task.needCb = false; // 避让时关闭吸球，避免抢球判定冲突
	}

	// 6. 组装控制任务
	task.target_pos = smooth_target_pos;

	// 车头永远死死盯住球
	task.orientate = (ball_pos - player_pos).angle();
	task.needKick = false;
	task.isPass = false;

	return task;
}

#endif