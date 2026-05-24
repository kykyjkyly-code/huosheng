#if 1
#include "src\utils\PlayerTask.h"
#include "src\getballsource.h"
#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"


/*
功能流程：
1. 本文件负责推球/传球前的拿球：先靠近球，再绕到目标方向后方，持续微调保持位置。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 里会自动选择对应参数。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== PGET调参区 ====================*/
// 没有队友时，站在球后方的额外距离。
const float REAL_PGET_NO_RECEIVER_BACK_EXTRA = 5.0f;
const float SIM_PGET_NO_RECEIVER_BACK_EXTRA = 5.0f;
// 固定目标点，沿用 jie.cpp 的接球点。
const float REAL_PGET_TARGET_POS_X = 100.0f;
const float REAL_PGET_TARGET_POS_Y = 130.0f;
const float SIM_PGET_TARGET_POS_X = 100.0f;
const float SIM_PGET_TARGET_POS_Y = 130.0f;
// 绕球半径在机器人半径基础上额外加的距离。
const float REAL_PGET_CIRCLE_EXTRA_DIST = 10.0f;
const float SIM_PGET_CIRCLE_EXTRA_DIST = 10.0f;
// 到达绕球圆周附近的容差。
                // 反向：目标点 → 球 → 小车
const float REAL_PGET_ARRIVE_CIRCLE_ERR = 1.0f;
const float SIM_PGET_ARRIVE_CIRCLE_ERR = 1.0f;
// 每一帧绕球调整的角度步长。
const float REAL_PGET_ORBIT_STEP_ANGLE = 0.25f;
const float SIM_PGET_ORBIT_STEP_ANGLE = 0.25f;
// 认为已经绕到合适角度的误差阈值（越小越精确）。
const float REAL_PGET_ALIGN_ANGLE = 0.15f;
const float SIM_PGET_ALIGN_ANGLE = 0.05f;
// 小车离球太远时重置状态机的额外距离。
const float REAL_PGET_RESET_FAR_EXTRA = 25.0f;
const float SIM_PGET_RESET_FAR_EXTRA = 25.0f;


/*==================== 功能块 3：拿球状态定义 ====================*/
/*
GO_TO_BALL：先靠近球
ORBIT_BALL：绕球调整位置，持续微调保持后方位置
*/
enum RotateBallState
{
	GO_TO_BALL = 0,     // 先面向球，跑到球附近
	ORBIT_BALL = 1     // 绕球，车头一直指向球，持续微调
};


/*==================== 功能块 4：状态变量 ====================*/
/*
state 记录当前执行到哪一步。
*/
static RotateBallState state = GO_TO_BALL;


PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	/*==================== 功能块 5：创建任务并读取基础信息 ====================*/
	/*
	获取当前小车位置、小车朝向、小球位置。
	后面所有目标点和方向都基于这些信息计算。
	*/
	PlayerTask task;

	if (model == NULL) {
		return task;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float no_receiver_back_extra = is_sim ? SIM_PGET_NO_RECEIVER_BACK_EXTRA : REAL_PGET_NO_RECEIVER_BACK_EXTRA;
	const float target_pos_x = is_sim ? SIM_PGET_TARGET_POS_X : REAL_PGET_TARGET_POS_X;
	const float target_pos_y = is_sim ? SIM_PGET_TARGET_POS_Y : REAL_PGET_TARGET_POS_Y;
	const float circle_extra_dist = is_sim ? SIM_PGET_CIRCLE_EXTRA_DIST : REAL_PGET_CIRCLE_EXTRA_DIST;
	const float arrive_circle_err_value = is_sim ? SIM_PGET_ARRIVE_CIRCLE_ERR : REAL_PGET_ARRIVE_CIRCLE_ERR;
	const float orbit_step_angle = is_sim ? SIM_PGET_ORBIT_STEP_ANGLE : REAL_PGET_ORBIT_STEP_ANGLE;
	const float align_angle_value = is_sim ? SIM_PGET_ALIGN_ANGLE : REAL_PGET_ALIGN_ANGLE;
	const float reset_far_extra = is_sim ? SIM_PGET_RESET_FAR_EXTRA : REAL_PGET_RESET_FAR_EXTRA;

	const point2f& player_pos = model->get_our_player_pos(robot_id);
	const float robotDir = model->get_our_player_dir(robot_id);
	const point2f& ball_pos = model->get_ball_pos();


	/*==================== 功能块 6：寻找接球队友 ====================*/
	/*
	从我方机器人中寻找一个非自己、非守门员的队友，
	作为传球或推球的目标。
	*/
	// 找一个接球队友。这里仍然沿用你的写法：选场上最后一个非自己、非守门员的我方车。
	int receiver_id = -1;

	for (int i = 0; i < 6; i++)
	{
		if (i == robot_id || i == model->get_our_goalie())
			continue;

		if (model->get_our_exist_id()[i])
			receiver_id = i;
	}


	/*==================== 功能块 7：没有接球队友时的处理 ====================*/
	/*
	如果没有找到接球队友，
	小车先移动到球附近，并保持车头朝向球。
	*/
	// 没有接球队员时，先到球后方，面向球
	if (receiver_id == -1)
	{
		float faceBallDir = (ball_pos - player_pos).angle();
		task.orientate = faceBallDir;
		task.target_pos = ball_pos - Maths::vector2polar(no_receiver_back_extra + MAX_ROBOT_SIZE, faceBallDir);
		task.needCb = true;
		return task;
	}


	/*==================== 功能块 8：计算传球方向和绕球目标方向 ====================*/
	/*
	传球方向为：球指向固定目标点。
	小车最终应该绕到球的反方向，
	形成"小车 —— 球 —— 目标点"的位置关系。
	*/
	const point2f target_point(target_pos_x, target_pos_y);

	// 球 -> 目标点方向，也就是 kicker 最终要朝向/推球的方向
	float passDir = (target_point - ball_pos).angle();

	// 车绕球时，最终应该站在球的反方向：
	// 目标点 ---- 球 ---- 小车
	float targetRobotRelDir = passDir;

	// 小车当前相对球的方向：球 -> 小车
	float currentRobotRelDir = (player_pos - ball_pos).angle();

	// 车到球的距离
	float distToBall = (player_pos - ball_pos).length();


	/*==================== 功能块 9：参数设置 ====================*/
	/*
	这些参数控制绕球半径和角度判断。
	主要用于调试小车是否绕得稳、角度是否精确。
	*/
	// 参数可以按仿真效果微调
	const float circleR = circle_extra_dist + MAX_ROBOT_SIZE;     // 绕球半径，表示球心到车中心的距离
	const float arriveCircleErr = arrive_circle_err_value;               // 到达圆周附近的容差
	const float DetAngle = orbit_step_angle;                            // 每次绕球角度步长

	// 每次绕球角度，越小越平滑，越大越快
	const float alignAngle = align_angle_value;                   // 角度对齐阈值，越小越精确

	task.needCb = true;   // 开吸球/控球

	/*==================== 功能块 10：异常距离重置 ====================*/
	/*
	如果小车离球太远，
	说明当前拿球状态可能已经失效，
	重新回到 GO_TO_BALL 阶段。
	*/
	// 如果球和车突然距离太远，重新进入拿球阶段
	if (distToBall > circleR + reset_far_extra)
	{
		state = GO_TO_BALL;
	}


	/*==================== 功能块 11：状态机控制 ====================*/
	/*
	两个状态：
	GO_TO_BALL — 靠近球
	ORBIT_BALL — 绕球持续微调，对齐后保持在目标位置
	*/
	switch (state)
	{
	case GO_TO_BALL:
	{
		/*==================== 状态 1：靠近球 ====================*/
		/*
		小车先面向球，并移动到球附近的圆周位置。
		到达指定距离后，进入绕球状态。
		*/
		// 面向球的方向
		float faceBallDir = (ball_pos - player_pos).angle();

		// 车头面向球，方便靠近
		task.orientate = faceBallDir;

		// 目标点：球前方 circleR 处，先靠近球
		task.target_pos = ball_pos - Maths::vector2polar(circleR, faceBallDir);

		if (distToBall <= circleR + arriveCircleErr)
		{
			state = ORBIT_BALL;
		}

		break;
	}

	case ORBIT_BALL:
	{
		/*==================== 状态 2：绕球持续微调 ====================*/
		/*
		小车沿着以球为圆心的圆运动，
		目标是绕到球的后方，
		让"小车 —— 球 —— 接球队友"对齐。
		对齐后持续微调，保持精确位置。
		*/
		// 绕球过程中车头始终指向球
		task.orientate = (ball_pos - player_pos).angle();

		// 判断当前"球->车"角度和目标"球->车"角度是否接近
		float err = Maths::normalizeAngle(targetRobotRelDir - currentRobotRelDir);

		if (fabs(err) < alignAngle)
		{
			// 角度已对齐，保持在目标位置持续微调
			task.target_pos = ball_pos + Maths::vector2polar(circleR, targetRobotRelDir);
			break;
		}

		// 按最短方向绕球
		float step = 0.0f;

		if (err > 0)
			step = DetAngle;
		else
			step = -DetAngle;

		// 防止最后一步绕过头
		if (fabs(err) < DetAngle)
			step = err;

		float nextRelDir = Maths::normalizeAngle(currentRobotRelDir + step);

		// 小车目标点仍在以球为圆心的圆上
		task.target_pos = ball_pos + Maths::vector2polar(circleR, nextRelDir);

		break;
	}

	default:
	{
		/*==================== 状态异常处理 ====================*/
		/*
		如果状态值异常，重新回到 GO_TO_BALL。
		*/
		state = GO_TO_BALL;
		break;
	}
	}

	return task;
}
#endif
