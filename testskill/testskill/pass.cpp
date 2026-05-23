#if 0
#include"src\utils\PlayerTask.h"
#include"src\getballsource.h"
#include"src\utils\worldmodel.h"
#include"src\utils\maths.h"
#include <cmath>

/*
功能流程：
1. 本文件负责传球队员拿球，稳定后执行传球。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 和 isget 会自动选择对应参数。
我更改的东西是判断他是否拿到球，两个角度的误差和官方规定的接球嘴中心的距离, 还有一些帧数来判断是否拿稳
还有如果没有拿到球的时候靠近球到球后方的那些距离
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 传球调参区 ====================*/
// 球在传球队员车头方向的最大角度误差。（放宽，原来实 0.07 / 仿 0.10）
const float REAL_PASS_MOUTH_ANGLE_THRESHOLD = 0.14f;
const float SIM_PASS_MOUTH_ANGLE_THRESHOLD = 0.18f;
// 拿球距离判定在 get_ball_threshold(15) 基础上额外放宽的距离。
const float REAL_PASS_GET_BALL_DIST_OFFSET = 5.0f;
const float SIM_PASS_GET_BALL_DIST_OFFSET = 5.0f;
// 固定传球目标点，和 jie.cpp 的接球点保持一致。
const float REAL_PASS_TARGET_POS_X = 20.0f;
const float REAL_PASS_TARGET_POS_Y = 70.0f;
const float SIM_PASS_TARGET_POS_X = 20.0f;
const float SIM_PASS_TARGET_POS_Y = 70.0f;
// 默认拿球时站在球后方的距离。
const float REAL_PASS_GET_BALL_BACK_DIST = 13.0f;
const float SIM_PASS_GET_BALL_BACK_DIST = 13.0f;
// 条件未满足时继续靠近球的距离。
const float REAL_PASS_APPROACH_BACK_DIST = 4.0f;
const float SIM_PASS_APPROACH_BACK_DIST = 15.0f;
// 连续满足控球条件多少帧后传球。（放宽，原来实 15 / 仿 20）
const int REAL_PASS_STABLE_FRAME = 8;
const int SIM_PASS_STABLE_FRAME = 1;
// 平射传球力度。
const double REAL_PASS_KICK_POWER = 25.0;
const double SIM_PASS_KICK_POWER = 25.0;


/*==================== 功能块 1：判断传球队员是否控到球 ====================*/
/*
只判断传球队员是否控到球：
- 球离自己足够近
- 球在自己车头方向

两个条件都满足，才返回 true。
*/
bool isget(const WorldModel* model, int robot_id)
{
	if (model == NULL) {
		return false;
	}

	if (robot_id < 0 || robot_id >= 6) {
		return false;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float mouth_angle_threshold = is_sim ? SIM_PASS_MOUTH_ANGLE_THRESHOLD : REAL_PASS_MOUTH_ANGLE_THRESHOLD;
	const float get_ball_dist_offset = is_sim ? SIM_PASS_GET_BALL_DIST_OFFSET : REAL_PASS_GET_BALL_DIST_OFFSET;

	// 获取传球队员坐标
	const point2f& player_pos = model->get_our_player_pos(robot_id);

	// 获取球的位置
	const point2f& ball_pos = model->get_ball_pos();

	// 获取传球队员朝向
	const float my_dir = model->get_our_player_dir(robot_id);

	// 小车到球的向量
	const point2f player_to_ball = ball_pos - player_pos;

	// 小车到球的距离
	const float ball_dist = player_to_ball.length();

	// 小车指向球的方向
	const float ball_dir = player_to_ball.angle();

	// 车头方向和球方向的角度差
	const float dir_error = fabs(Maths::normalizeAngle(ball_dir - my_dir));

	// 判断球是否离小车足够近
	const bool ball_near = ball_dist < get_ball_threshold + get_ball_dist_offset+100.0f;

	// 判断球是否在小车车头方向
	const bool ball_in_front = dir_error < mouth_angle_threshold;

	// 传球队员是否已经控到球
	return ball_near && ball_in_front;
}


/*==================== 功能块 2：任务初始化 ====================*/
/*
创建任务对象，并判断世界模型数据是否有效。
*/
PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	PlayerTask task;

	if (model == NULL) {
		return task;
	}

	// 防止 robot_id 越界
	if (robot_id < 0 || robot_id >= 6)
	{
		return task;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float target_pos_x = is_sim ? SIM_PASS_TARGET_POS_X : REAL_PASS_TARGET_POS_X;
	const float target_pos_y = is_sim ? SIM_PASS_TARGET_POS_Y : REAL_PASS_TARGET_POS_Y;
	const float get_ball_back_dist = is_sim ? SIM_PASS_GET_BALL_BACK_DIST : REAL_PASS_GET_BALL_BACK_DIST;
	const float approach_back_dist = is_sim ? SIM_PASS_APPROACH_BACK_DIST : REAL_PASS_APPROACH_BACK_DIST;
	const int stable_frame = is_sim ? SIM_PASS_STABLE_FRAME : REAL_PASS_STABLE_FRAME;
	const double kick_power = is_sim ? SIM_PASS_KICK_POWER : REAL_PASS_KICK_POWER;


	/*==================== 功能块 3：寻找接球队员 ====================*/
	/*
	从我方机器人中选择一台非自己、非守门员的车作为接球队员。
	找到后保存它的编号 receiver_id。
	*/
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


	/*==================== 功能块 4：没有接球队员时返回 ====================*/
	/*
	如果没有找到接球队员，直接返回空任务。
	此时小车不执行拿球或传球动作。
	*/
	if (receiver_id == -1)
	{
		return task;
	}


	/*==================== 功能块 5：获取场上关键信息 ====================*/
	/*
	获取当前小车、球、固定目标点的位置。
	后面用这些位置计算拿球点和传球方向。
	*/
	const point2f& player_pos = model->get_our_player_pos(robot_id);

	const point2f& ball_pos = model->get_ball_pos();

	const point2f target_point(target_pos_x, target_pos_y);


	/*==================== 功能块 6：计算传球方向 ====================*/
	/*
	传球方向为：球指向接球队员车头（控球嘴位置）。
	小车拿球时也保持这个方向，方便拿到球后直接传球。
	*/
	const point2f& receiver_pos = model->get_our_player_pos(receiver_id);
	const float receiver_dir = model->get_our_player_dir(receiver_id);
	const point2f receiverHeadPos = receiver_pos + Maths::vector2polar(static_cast<float>(ROBOT_HEAD), receiver_dir);
	float face_dir = (receiverHeadPos - ball_pos).angle();


	/*==================== 功能块 7：设置默认拿球任务 ====================*/
	/*
	默认动作：
	小车移动到球后方，车头朝向固定目标点，打开吸球。
	此时还不踢球。
	*/
	task.orientate = face_dir;
	task.target_pos = ball_pos - Maths::vector2polar(get_ball_back_dist, face_dir);

	task.needCb = true;

	task.needKick = false;
	task.isPass = false;


	/*==================== 功能块 8：控球稳定后再传球 ====================*/

	// 这个 static 计数器要放在 player_plan 函数里面，但不能放进 if 里面
	// 作用：记录每台车连续满足"控球"条件的帧数
	static int hold_cnt[6] = { 0 };

	// 当前这一帧是否满足：传球队员控到球
	bool get_ball = isget(model, robot_id);

	// 如果这一帧满足条件，计数 +1
	// 如果这一帧不满足条件，计数清零
	if (get_ball)
	{
		hold_cnt[robot_id]++;
	}
	else
	{
		hold_cnt[robot_id] = 0;
	}

	// 稳定帧数阈值
	// 10 帧比较快，20 帧更稳但会慢一点
	

	// 是否已经稳定满足条件
	bool stable_get_ball = hold_cnt[robot_id] >= stable_frame;


	/*==================== 情况 1：已经稳定控球，允许传球 ====================*/
	if (stable_get_ball)
	{
		// 稳定控球后停在当前位置，不要继续往球上顶
		task.target_pos = player_pos;

		// 面向接球队员
		task.orientate = face_dir;

		// 打开吸球
		task.needCb = true;

		// 平射传球，不是挑射
		task.isChipKick = false;

		// 开启击球
		task.needKick = true;

		// 标记这是传球
		task.isPass = true;

		// 传球力度
		task.kickPower = kick_power;
	}


	/*==================== 情况 2：刚控到球，但还没稳定，先不踢 ====================*/
	else if (get_ball)
	{
		// 球已经在嘴边，但还没有连续稳定 enough 帧
		// 所以先不踢，继续吸住等待

		task.target_pos = player_pos;

		task.orientate = face_dir;

		task.needCb = true;

		task.needKick = false;
		task.isPass = false;

		task.isChipKick = false;
	}


	/*==================== 情况 3：还没控到球，继续拿球 ====================*/
	else
	{
		// 如果还没有控到球，就继续去球后方拿球

		task.orientate = face_dir;

		// 8 比 5 稍微保守一点，不容易顶球
		task.target_pos = ball_pos - Maths::vector2polar(approach_back_dist, face_dir);

		task.needCb = true;

		task.needKick = false;
		task.isPass = false;
		task.isChipKick = false;
	}


	/*==================== 功能块 10：返回任务 ====================*/
	/*
	返回本帧任务，系统根据 target_pos、orientate、needCb、needKick 等参数控制小车。
	*/
	return task;
}
#endif
