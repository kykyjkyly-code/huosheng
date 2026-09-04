#if 0

#include "src\utils\PlayerTask.h"
#include "src\getballsource.h"
#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"

/*
功能流程：
1. 本文件负责二次接球，先站到固定接球点，再根据来球方向调整车头。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 里会自动选择对应参数。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 二次接球调参区 ====================*/
// 固定接球点。
const float REAL_JIE2CI_RECEIVE_POS_X = 140.0f;
const float REAL_JIE2CI_RECEIVE_POS_Y = -80.0f;
const float SIM_JIE2CI_RECEIVE_POS_X = 140.0f;
const float SIM_JIE2CI_RECEIVE_POS_Y = -80.0f;
// 球离开发球车超过这个距离后，认为球已经传出来。
const float REAL_JIE2CI_BALL_LEAVE_DIST_EXTRA = 8.0f;
const float SIM_JIE2CI_BALL_LEAVE_DIST_EXTRA = 8.0f;


PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	/*==================== 功能块 2：初始化任务 ====================*/
	/*
	创建任务对象，并判断视觉/世界模型数据是否有效。，明确里一台
	*/
	PlayerTask task;

	if (model == NULL) {
		return task;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float receive_pos_x = is_sim ? SIM_JIE2CI_RECEIVE_POS_X : REAL_JIE2CI_RECEIVE_POS_X;
	const float receive_pos_y = is_sim ? SIM_JIE2CI_RECEIVE_POS_Y : REAL_JIE2CI_RECEIVE_POS_Y;
	const float ball_leave_dist_extra = is_sim ? SIM_JIE2CI_BALL_LEAVE_DIST_EXTRA : REAL_JIE2CI_BALL_LEAVE_DIST_EXTRA;

	int receiver_id = -1;

	for (int i = 0; i < 6; i++)
	{
		if (i == robot_id || i == model->get_our_goalie())
			continue;

		if (model->get_our_exist_id()[i])
			receiver_id = i;
	}


	/*==================== 功能块 3：获取场上信息 ====================*/
	/*
	获取机器人当前位置、球的位置和球的速度。
	receive_pos 当前写法等于机器人当前位置，相当于原地等待点。
	*/
	point2f receive_pos(receive_pos_x, receive_pos_y);

	const point2f& receiver_pos = model->get_our_player_pos(robot_id);
	const point2f& ball_pos = model->get_ball_pos();
	const point2f& ball_vel = model->get_ball_vel();


	/*==================== 功能块 4：设置默认任务 ====================*/
	/*
	默认目标点为接球点，默认开启吸球，不主动踢球,角度是接球车到球的方向
	*/
	task.target_pos = receive_pos;//可以要把这个给他改为固定点位



	task.needCb = true;
	task.needKick = false;
	task.isPass = false;

	float face_dir = 0.0f;
	face_dir =- (receiver_pos - ball_pos).angle();
	task.orientate = face_dir;
	/*==================== 功能块 5：判断球是否传来 ====================*/

	//根据球离发球球员的距离来判断

	const point2f& kicker_pos = model->get_our_player_pos(receiver_id);

	// 球到踢球球员的距离
	float ball_to_kicker_dist = (ball_pos - kicker_pos).length();

	// 当球离开踢球球员一定距离，认为球已经被传出来
	bool ball_is_coming = ball_to_kicker_dist > MAX_ROBOT_SIZE + ball_leave_dist_extra;
	/*==================== 功能块 6：动态接球 ====================*/
	/*
	球在运动时：
	根据球的运动方向计算接球路线，
	让小车移动到球路附近，并面向来球方向接球。
	*/
	if (ball_is_coming)
	{
		float ballMoveDir = ball_vel.angle();

		//	point2f task_point = Maths::line_perp_across(
		//	ball_pos,
		//	ballMoveDir,
		//	receive_pos
		//};





		//这个应该要修改
		//task.target_pos =
		//	task_point;// + Maths::vector2polar(MAX_ROBOT_SIZE + 3.0f, ballMoveDir);
		//
		face_dir = Maths::normalizeAngle(ballMoveDir);
	}


	/*==================== 功能块 7：等待接球 ====================*/
	/*
	球没有明显运动时：
	小车停在接球点等待，并保持车头朝向小球。
	*/
	else
	{
		task.target_pos = receive_pos;
		face_dir =-(receiver_pos - ball_pos).angle();
	}


	/*==================== 功能块 8：输出任务 ====================*/
	/*
	设置最终车头方向，并返回任务给系统执行。
	*/
	task.orientate = face_dir;

	return task;
}
#endif
