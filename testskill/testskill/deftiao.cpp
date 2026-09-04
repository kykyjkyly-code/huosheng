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

	// 1. 获取防守车自身的基本位置
	const point2f& player_pos = model->get_our_player_pos(robot_id);
	const point2f& ball_pos = model->get_ball_pos();

	// 【刚性指定】根据你的实地要求：1号是敌方Kicker，2号是敌方Receiver
	const int TARGET_KICKER_ID = 1;
	const int TARGET_RECEIVER_ID = 2;

	// 验证这两辆车在实地上是否都存在
	bool kicker_exist = model->get_opp_exist_id()[TARGET_KICKER_ID];
	bool receiver_exist = model->get_opp_exist_id()[TARGET_RECEIVER_ID];

	point2f raw_target_pos;

	if (kicker_exist && receiver_exist)
	{
		// 【战术核心】：刚性获取敌方 1 号车和 2 号车的实时坐标 
		const point2f& attacker_kicker = model->get_opp_player_pos(TARGET_KICKER_ID);
		const point2f& attacker_receiver = model->get_opp_player_pos(TARGET_RECEIVER_ID);

		// 精准锁死两车连线的 1/2 正中点
		raw_target_pos = (attacker_kicker + attacker_receiver) * 0.5f;
	}
	else if (kicker_exist)
	{
		// 容错：如果2号没上场，缓慢跟着1号后面呆着
		raw_target_pos = model->get_opp_player_pos(TARGET_KICKER_ID) - Maths::vector2polar(40.0f, 0.0f);
	}
	else
	{
		// 最终保底：如果人都不在，原地不动
		raw_target_pos = player_pos;
	}

	// 2. 【极速阻尼过滤】：解决实地跑得太快的问题
	// 将系数降低到 0.06f。它每帧只会极其佛系、极其缓慢地朝着中点蹭过去，绝对不会猛冲
	float speed_reducer = 0.06f;
	point2f smooth_target_pos = player_pos + (raw_target_pos - player_pos) * speed_reducer;

	// 3. 【实地秒怂防碰撞机制】：彻底解决对撞、抢球问题
	// 敏感避让半径（两个车身大小加余量），只要有人逼近，防守车执行"主动倒车/闪开"
	const float AVOID_DIST = MAX_ROBOT_SIZE * 2.5f;
	bool need_avoid = false;
	point2f danger_opp_pos;

	for (int i = 0; i < 6; i++)
	{
		if (model->get_opp_exist_id()[i])
		{
			const point2f& opp_pos = model->get_opp_player_pos(i); // 
			float dist_to_me = (player_pos - opp_pos).length();
			float dist_to_target = (smooth_target_pos - opp_pos).length();

			// 如果对方任何人离我太近，或者正好挡在我的目的地慢移线上
			if (dist_to_me < AVOID_DIST || dist_to_target < AVOID_DIST)
			{
				need_avoid = true;
				danger_opp_pos = opp_pos; // 记录危险来源
				break;
			}
		}
	}

	// 执行避让逻辑
	if (need_avoid)
	{
		// 向量朝向：从危险源指向我的相反方向（即被对方逼退，顺着方向向后倒车逃跑，绝不触碰）
		float escape_dir = (player_pos - danger_opp_pos).angle();
		smooth_target_pos = player_pos + Maths::vector2polar(AVOID_DIST, escape_dir);
	}

	// 4. 组装控制任务
	task.target_pos = smooth_target_pos;

	// 车头朝向：如果1号存在，看着1号持球车发呆；否则看球 [cite: 3125, 3130]
	if (kicker_exist) {
		task.orientate = (model->get_opp_player_pos(TARGET_KICKER_ID) - player_pos).angle(); // 
	}
	else {
		task.orientate = (ball_pos - player_pos).angle(); // [cite: 3125]
	}

	// 彻底关闭所有主动性抢球/击球开关
	task.needCb = false; // [cite: 3180]
	task.needKick = false; // [cite: 3171]
	task.isPass = false; // [cite: 3173]

	return task;
}