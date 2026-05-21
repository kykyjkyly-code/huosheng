#if 0
#include"src\utils\PlayerTask.h"
#include"src\getballsource.h"
#include"src\utils\worldmodel.h"
#include"src\utils\maths.h"
#include <cmath>

/*
功能流程：
1. 本文件负责传球队员拿球、等待接球队员朝向正确，然后执行传球。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 和 isget 会自动选择对应参数。
我更改的东西是判断他是否拿到球，两个角度的误差和官方规定的接球嘴中心的距离, 还有一些帧数来判断是否拿稳
还有如果没有拿到球的时候靠近球到球后方的那些距离
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 传球调参区 ====================*/
// 球在传球队员车头方向的最大角度误差。
const float REAL_PASS_MOUTH_ANGLE_THRESHOLD = 0.07f;
const float SIM_PASS_MOUTH_ANGLE_THRESHOLD = 0.10f;
// 接球队员面对来球方向的最大角度误差。
const float REAL_PASS_RECEIVER_FACE_THRESHOLD = 0.10f;
const float SIM_PASS_RECEIVER_FACE_THRESHOLD = 0.10f;
// 默认拿球时站在球后方的距离。
const float REAL_PASS_GET_BALL_BACK_DIST = 13.0f;
const float SIM_PASS_GET_BALL_BACK_DIST = 13.0f;
// 条件未满足时继续靠近球的距离。
const float REAL_PASS_APPROACH_BACK_DIST = 4.0f;
const float SIM_PASS_APPROACH_BACK_DIST = 5.0f;
// 连续满足控球和接球队员朝向条件多少帧后传球。
const int REAL_PASS_STABLE_FRAME = 15;
const int SIM_PASS_STABLE_FRAME = 20;
// 平射传球力度。
const double REAL_PASS_KICK_POWER = 25.0;
const double SIM_PASS_KICK_POWER = 25.0;


/*==================== 功能块 0：角度处理 ====================*/
/*
将角度限制在 [-PI, PI] 范围内。

原因：
角度是一个圆，比如 3.13 和 -3.13 实际上几乎是同一个方向。
如果直接相减，会误判成差了很多。
*/
float normalizeAngle(float angle)
{
	while (angle > PI) angle -= 2 * PI;
	while (angle < -PI) angle += 2 * PI;
	return angle;
}


/*==================== 功能块 1：判断是否控到球，并判断接球队员朝向 ====================*/
/*
这个函数现在判断两件事：

1. 当前传球队员是否控到球：
- 球离自己足够近
- 球在自己车头方向

2. 接球队员是否朝向正确：
- 接球队员应该面对来球方向
- 也就是传球方向 pass_dir 的反方向，即 pass_dir + PI

只有两个条件都满足，才返回 true。
*/
bool isget(const WorldModel* model, int robot_id, int receiver_id, float pass_dir)
{
	if (model == NULL) {
		return false;
	}

	if (robot_id < 0 || robot_id >= 6) {
		return false;
	}

	if (receiver_id < 0 || receiver_id >= 6) {
		return false;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float mouth_angle_threshold = is_sim ? SIM_PASS_MOUTH_ANGLE_THRESHOLD : REAL_PASS_MOUTH_ANGLE_THRESHOLD;
	const float receiver_face_threshold = is_sim ? SIM_PASS_RECEIVER_FACE_THRESHOLD : REAL_PASS_RECEIVER_FACE_THRESHOLD;

	/*==================== 1. 判断传球队员是否控到球 ====================*/

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
	// 这里必须使用 normalizeAngle，防止 PI 和 -PI 边界误判
	const float dir_error = fabs(normalizeAngle(ball_dir - my_dir));

	// 角度阈值：球必须在车头前方
	

	// 判断球是否离小车足够近
	const bool ball_near = ball_dist < get_ball_threshold;

	// 判断球是否在小车车头方向
	const bool ball_in_front = dir_error < mouth_angle_threshold;

	// 传球队员是否已经控到球
	const bool kicker_get_ball = ball_near && ball_in_front;


	/*==================== 2. 判断接球队员朝向是否正确 ====================*/

	// 获取接球队员当前朝向
	const float receiver_dir = model->get_our_player_dir(receiver_id);

	// pass_dir 是传球方向：球 -> 接球队员
	// 接球队员接球时应该面对来球，所以方向应该是 pass_dir + PI
	const float receiver_should_dir = normalizeAngle(pass_dir + PI);

	// 接球队员当前朝向和应该朝向之间的误差
	const float receiver_dir_error = fabs(normalizeAngle(receiver_dir - receiver_should_dir));

	// 接球队员朝向误差阈值
	// 0.10 弧度约等于 5.7 度，可以根据实际效果调大或调小
	

	// 接球队员朝向是否准备好
	const bool receiver_face_ready = receiver_dir_error < receiver_face_threshold;


	/*==================== 3. 两个条件都满足，才认为可以传球 ====================*/

	return kicker_get_ball && receiver_face_ready;
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
	获取当前小车、球、接球队员的位置。
	后面用这些位置计算拿球点和传球方向。
	*/
	const point2f& player_pos = model->get_our_player_pos(robot_id);

	const point2f& ball_pos = model->get_ball_pos();

	const point2f& receiver_pos = model->get_our_player_pos(receiver_id);


	/*==================== 功能块 6：计算传球方向 ====================*/
	/*
	传球方向为：球指向接球队员。
	小车拿球时也保持这个方向，方便拿到球后直接传球。
	*/
	float face_dir = (receiver_pos - ball_pos).angle();


	/*==================== 功能块 7：设置默认拿球任务 ====================*/
	/*
	默认动作：
	小车移动到球后方，车头朝向接球队员，打开吸球。
	此时还不踢球。
	*/
	task.orientate = face_dir;
	task.target_pos = ball_pos - Maths::vector2polar(get_ball_back_dist, face_dir);

	task.needCb = true;

	task.needKick = false;
	task.isPass = false;


	/*==================== 功能块 8：控球稳定后再传球 ====================*/

	// 这个 static 计数器要放在 player_plan 函数里面，但不能放进 if 里面
	// 作用：记录每台车连续满足“控球 + 接球队员朝向正确”的帧数
	static int hold_cnt[6] = { 0 };

	// 当前这一帧是否满足：
	// 1. 传球队员控到球
	// 2. 接球队员朝向已经对准来球方向
	bool get_ball = isget(model, robot_id, receiver_id, face_dir);

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


	/*==================== 情况 1：已经稳定控球，并且接球队员朝向正确，允许传球 ====================*/
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


	/*==================== 情况 2：条件刚满足，但还没稳定，不踢 ====================*/
	else if (get_ball)
	{
		// 球已经在嘴边，并且接球队员朝向也对了
		// 但还没有连续稳定 enough 帧
		// 所以先不踢，继续吸住等待

		task.target_pos = player_pos;

		task.orientate = face_dir;

		task.needCb = true;

		task.needKick = false;
		task.isPass = false;

		task.isChipKick = false;
	}


	/*==================== 情况 3：未满足条件，继续拿球或等待接球队员对准 ====================*/
	else
	{
		// 如果还没有控到球，或者接球队员朝向还没对准
		// 就继续去球后方拿球，并保持朝向接球队员

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
