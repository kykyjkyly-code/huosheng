#if 0
#include "src\utils\PlayerTask.h"
#include "src\getballsource.h"
#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"


/*
功能流程：
1. 本文件负责推球/传球前的拿球：先靠近球，再绕到目标方向后方，最后慢慢贴近球。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 里会自动选择对应参数。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== PGET调参区 ====================*/
// 没有队友时，站在球后方的额外距离。
const float REAL_PGET_NO_RECEIVER_BACK_EXTRA = 5.0f;
const float SIM_PGET_NO_RECEIVER_BACK_EXTRA = 5.0f;
// 绕球半径在机器人半径基础上额外加的距离。
const float REAL_PGET_CIRCLE_EXTRA_DIST = 10.0f;
const float SIM_PGET_CIRCLE_EXTRA_DIST = 10.0f;
// 到达绕球圆周附近的容差。
const float REAL_PGET_ARRIVE_CIRCLE_ERR = 4.0f;
const float SIM_PGET_ARRIVE_CIRCLE_ERR = 4.0f;
// 每一帧绕球调整的角度步长。
const float REAL_PGET_ORBIT_STEP_ANGLE = 0.25f;
const float SIM_PGET_ORBIT_STEP_ANGLE = 0.25f;
// 认为已经绕到合适角度的误差阈值。
const float REAL_PGET_ALIGN_ANGLE = 0.5f;
const float SIM_PGET_ALIGN_ANGLE = 0.5f;
// 慢慢靠近球时的初始距离。
const float REAL_PGET_CLOSE_START_EXTRA = 5.0f;
const float SIM_PGET_CLOSE_START_EXTRA = 5.0f;
// 每次靠近球减少的距离。
const float REAL_PGET_CLOSE_STEP = 0.35f;
const float SIM_PGET_CLOSE_STEP = 0.35f;
// 到达当前靠近点的判断距离。
const float REAL_PGET_CLOSE_TARGET_ERR = 1.5f;
const float SIM_PGET_CLOSE_TARGET_ERR = 1.5f;
// 小车离球太远时重置状态机的额外距离。
const float REAL_PGET_RESET_FAR_EXTRA = 25.0f;
const float SIM_PGET_RESET_FAR_EXTRA = 25.0f;


/*==================== 功能块 3：拿球状态定义 ====================*/
/*
GO_TO_BALL：先靠近球
ORBIT_BALL：绕球调整位置
CLOSE_BALL：角度合适后慢慢靠近球
*/
enum RotateBallState
{
	GO_TO_BALL = 0,     // 先面向球，跑到球附近0
	ORBIT_BALL = 1,    // 绕球，车头一直指向球
	CLOSE_BALL = 2     // 角度合适后，慢慢靠近球
};


/*==================== 功能块 4：状态变量 ====================*/
/*
state 记录当前执行到哪一步。
closeDist 记录靠近球时，目标点离球的距离。
*/
static RotateBallState state = GO_TO_BALL;
static float closeDist = 0.0f;


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
	const float circle_extra_dist = is_sim ? SIM_PGET_CIRCLE_EXTRA_DIST : REAL_PGET_CIRCLE_EXTRA_DIST;
	const float arrive_circle_err_value = is_sim ? SIM_PGET_ARRIVE_CIRCLE_ERR : REAL_PGET_ARRIVE_CIRCLE_ERR;
	const float orbit_step_angle = is_sim ? SIM_PGET_ORBIT_STEP_ANGLE : REAL_PGET_ORBIT_STEP_ANGLE;
	const float align_angle_value = is_sim ? SIM_PGET_ALIGN_ANGLE : REAL_PGET_ALIGN_ANGLE;
	const float close_start_extra = is_sim ? SIM_PGET_CLOSE_START_EXTRA : REAL_PGET_CLOSE_START_EXTRA;
	const float close_step_value = is_sim ? SIM_PGET_CLOSE_STEP : REAL_PGET_CLOSE_STEP;
	const float close_target_err_value = is_sim ? SIM_PGET_CLOSE_TARGET_ERR : REAL_PGET_CLOSE_TARGET_ERR;
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
	根据接球队友位置，计算球到接球队友的方向。
	小车最终应该绕到球的反方向，
	形成“小车 —— 球 —— 接球队友”的位置关系。
	*/
	const point2f& receiver_pos = model->get_our_player_pos(receiver_id);

	// 球 -> 接球队员方向，也就是你要传球/推球的方向
	float passDir = (receiver_pos - ball_pos).angle();

	// 车绕球时，最终应该站在球的反方向：
	// 接球队员 ---- 球 ---- 小车
	float targetRobotRelDir = Maths::normalizeAngle(passDir + static_cast<float>(PI));

	// 小车当前相对球的方向：球 -> 小车
	float currentRobotRelDir = (player_pos - ball_pos).angle();

	// 车到球的距离
	float distToBall = (player_pos - ball_pos).length();


	/*==================== 功能块 9：参数设置 ====================*/
	/*
	这些参数控制绕球半径、角度判断、靠近速度和最小靠近距离。
	主要用于调试小车是否绕得稳、靠近是否太快。
	*/
	// 参数可以按仿真效果微调
	const float circleR = circle_extra_dist + MAX_ROBOT_SIZE;     // 绕球半径，表示球心到车中心的距离
	const float arriveCircleErr = arrive_circle_err_value;               // 到达圆周附近的容差
	const float DetAngle = orbit_step_angle;


	//在这里修改角度问题








	// 每次绕球角度，越小越平滑，越大越快
	const float alignAngle = align_angle_value;                   // 判断“角度合适”的阈值，约 28度
	const float closeStartDist = ROBOT_HEAD + BALL_SIZE + 7.0f;   // 比 closeMinDist 大5，给渐进靠近留空间






	//这里是距离问题
	const float closeMinDist = ROBOT_HEAD + BALL_SIZE + 2.0f;   // 14.5，吸球嘴碰到球面+缓冲，避免推球
	const float closeStep = close_step_value;                    // 每帧靠近距离，越小越慢

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
		closeDist = closeStartDist;
	}


	/*==================== 功能块 11：状态机控制 ====================*/
	/*
	根据当前状态，决定小车执行：
	靠近球、绕球、还是慢慢靠近吸球。
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
		// 一开始小车面向球去拿球
		float faceBallDir = (ball_pos - player_pos).angle();
		task.orientate = faceBallDir;

		// 不直接冲到球心，而是停在球前方一定距离
		task.target_pos = ball_pos - Maths::vector2polar(circleR, faceBallDir);

		if (distToBall <= circleR + arriveCircleErr)
		{
			state = ORBIT_BALL;
		}

		break;
	}

	case ORBIT_BALL:
	{
		/*==================== 状态 2：绕球调整角度 ====================*/
		/*
		小车沿着以球为圆心的圆运动，
		目标是绕到球的后方，
		让“小车 —— 球 —— 接球队友”对齐。
		*/
		// 绕球过程中，车头始终指向球
		task.orientate = (ball_pos - player_pos).angle();

		// 判断当前“球->车”角度和目标“球->车”角度是否接近
		float err = Maths::normalizeAngle(targetRobotRelDir - currentRobotRelDir);

		if (fabs(err) < alignAngle)
		{
			// 角度已经合适，进入慢慢靠近球阶段
			state = CLOSE_BALL;
			closeDist = closeStartDist;
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

	case CLOSE_BALL:
	{
		/*==================== 状态 3：慢慢靠近球 ====================*/
		/*
		小车已经绕到合适方向后，
		沿着传球方向的反方向逐步靠近球，
		同时保持车头朝向接球队友方向。
		*/
		// 进入靠近阶段后，仍然保持车头指向球到接球的球员
		task.orientate = (receiver_pos - ball_pos).angle();

		// 如果角度偏离太多，回到绕球阶段重新校正
		float err = Maths::angleDiff(currentRobotRelDir, targetRobotRelDir);

		if (err > alignAngle * 2.5f)
		{
			state = ORBIT_BALL;
			closeDist = closeStartDist;
			break;
		}

		// 目标点在“球和接球队员的直线”上：
		// receiver_pos ---- ball_pos ---- target_pos
		//task.target_pos = ball_pos - Maths::vector2polar(closeDist, passDir);




		point2f closeTarget = ball_pos - Maths::vector2polar(closeDist, passDir);
		task.target_pos = closeTarget;

		// 渐进减速，靠近球时逐渐降低速度，避免推球
		float distToFinal = closeDist - closeMinDist;
		if (distToFinal < 10.0f) {
			float speedRatio = distToFinal / 10.0f;
			if (speedRatio < 0.0f) speedRatio = 0.0f;
			float maxSpeed = speedRatio * 200.0f + 5.0f;
			point2f moveVec = closeTarget - player_pos;
			float moveLen = moveVec.length();
			if (moveLen > 0.01f) {
				moveVec = moveVec / moveLen;
				if (moveLen > maxSpeed / 60.0f) {
					task.global_vel = moveVec * maxSpeed;
				}
			}
			if (distToFinal <= 0.3f) {
				task.global_vel = point2f(0, 0);
			}
		}



		/*==================== 状态 3.1：分段靠近 ====================*/
		/*
		小车先到当前 closeTarget，
		到达后再减小 closeDist，
		这样可以避免每一帧都靠近导致直接推球。
		*/
		// 小车先到达当前靠近点，再缩短下一次距离
		const float closeTargetErr = close_target_err_value;   // 可调：1~3
		if ((player_pos - closeTarget).length() < closeTargetErr)
		{
			if (closeDist > closeMinDist)
			{
				closeDist -= closeStep;

				if (closeDist < closeMinDist)
					closeDist = closeMinDist;
			}
		}



		// 慢慢减少距离，让小车逐步靠近小球
		//if (closeDist > closeMinDist)
		//	{
		//		closeDist -= closeStep;
		//
		//	if (closeDist < closeMinDist)
		//		closeDist = closeMinDist;
		//	}

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
