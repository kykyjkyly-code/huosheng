#include"utils\PlayerTask.h"
#include"getballsource.h"
#include"utils\worldmodel.h"
#include"utils\maths.h"
#include <cmath>

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);


// 修改后的 isget 函数：支持动态传入角度阈值
bool isget_dynamic(const WorldModel* model, int robot_id, float angle_threshold)
{
	// 获取球员坐标
	const point2f& player_pos = model->get_our_player_pos(robot_id);

	// 获取球的位置
	const point2f& ball_pos = model->get_ball_pos();

	// 获取球员朝向
	const float my_dir = model->get_our_player_dir(robot_id);

	// 小车到球的向量
	const point2f player_to_ball = ball_pos - player_pos;

	// 小车到球的距离
	const float ball_dist = player_to_ball.length();

	// 小车指向球的方向
	const float ball_dir = player_to_ball.angle();

	// 车头方向和球方向的角度差
	const float dir_error = fabs(ball_dir - my_dir);

	// 判断球是否离小车足够近
	const bool ball_near = ball_dist < get_ball_threshold;

	// 使用动态传入的角度阈值（控球时放宽）
	const bool ball_in_front = dir_error < angle_threshold;

	return ball_near && ball_in_front;
}


PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	PlayerTask task;

	if (model == NULL) {
		return task;
	}

	// 找一个接球队员
	int receiver_id = -1;

	for (int i = 0; i < 6; i++)
	{
		// 不选自己，也不选守门员
		if (i == robot_id || i == model->get_our_goalie())
			continue;

		// 找到一个存在的我方球员
		if (model->get_our_exist_id()[i])
		{
			receiver_id = i;
			break;
		}
	}

	// 如果没有找到接球队员，就先不动
	if (receiver_id == -1)
	{
		return task;
	}

	// 获取传球队员坐标
	const point2f& player_pos = model->get_our_player_pos(robot_id);

	// 获取球的位置
	const point2f& ball_pos = model->get_ball_pos();

	// 获取接球队员的位置
	const point2f& receiver_pos = model->get_our_player_pos(receiver_id);

	// 方向指向接球队员
	float face_dir = (receiver_pos - ball_pos).angle();

	// 默认：去球的后方，车头朝向接球队员
	task.orientate = face_dir;
	task.target_pos = ball_pos - Maths::vector2polar(13, face_dir);

	// 默认打开吸球
	task.needCb = true;

	// 默认先不踢
	task.needKick = false;
	task.isPass = false;


	/*====================  ====================*/
	static int get_ball_frames = 0;   // 记录连续吸住球的帧数
	const int KICK_DELAY_FRAMES = 2;  // 连续2帧吸稳对准，立刻传球

	// 动态角度阈值：进嘴（>0帧）放宽到 0.12f 允许微调，没进嘴时严格要求 0.05f 保证对齐
	float current_angle_threshold = (get_ball_frames > 0) ? 0.12f : 0.05f;

	if (isget_dynamic(model, robot_id, current_angle_threshold))
	{
		get_ball_frames++;
	}
	else
	{
		// 不满足条件时直接清零
		get_ball_frames = 0;
	}


	/*==================== 状态机逻辑判定 ====================*/
	if (get_ball_frames >= KICK_DELAY_FRAMES)
	{
		// 传球瞬间给球前方的微小目标点，小车会顺着冲力把球“推射”出去
		task.target_pos = ball_pos + Maths::vector2polar(2, face_dir);
		task.orientate = face_dir;

		task.needCb = true;
		task.isChipKick = false;
		task.needKick = true; 
		task.isPass = true;
		task.kickPower = 35;
	}
	else
	{
		task.orientate = face_dir;

		task.target_pos = ball_pos - Maths::vector2polar(5, face_dir);

		task.needCb = true;
		task.needKick = false;
		task.isPass = false;
	}

	return task;
}