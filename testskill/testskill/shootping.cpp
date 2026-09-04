#if 0
#include"src\utils\PlayerTask.h"
#include"src\getballsource.h"
#include"src\utils\worldmodel.h"
#include"src\utils\maths.h"
#include <cmath>

/*
功能流程：
1. 本文件负责平射：根据对方守门员位置选择球门空当，控到球后执行平射。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 和 isget 会自动选择对应参数。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 平射调参区 ====================*/
// 球在车头方向的最大角度误差，单位是弧度。
const float REAL_SHOOTPING_MOUTH_ANGLE_THRESHOLD = 0.15f;
const float SIM_SHOOTPING_MOUTH_ANGLE_THRESHOLD = 0.05f;
// 默认拿球时站在球后方的距离。
const float REAL_SHOOTPING_GET_BALL_BACK_DIST = 13.0f;
const float SIM_SHOOTPING_GET_BALL_BACK_DIST = 13.0f;
// 控到球后贴近球的位置距离。
const float REAL_SHOOTPING_READY_BACK_DIST = 1.0f;
const float SIM_SHOOTPING_READY_BACK_DIST = 1.0f;
// 没控到球时继续靠近球的距离。
const float REAL_SHOOTPING_APPROACH_BACK_DIST = 5.0f;
const float SIM_SHOOTPING_APPROACH_BACK_DIST = 5.0f;
// 平射力度。
const double REAL_SHOOTPING_KICK_POWER = 30.0;
const double SIM_SHOOTPING_KICK_POWER = 30.0;
// 射门目标离上下门柱的安全距离，避免瞄准门柱或轻微误差导致出界。
const float REAL_SHOOTPING_POST_MARGIN = 8.0f;
const float SIM_SHOOTPING_POST_MARGIN = 8.0f;


//==================== 功能块 1：判断是否控到球 ====================
// 根据球和小车的位置、距离以及本帧选出的射门方向，
// 判断机器人是否已经具备射门条件。
bool isget(const WorldModel* model, int robot_id, float shoot_dir)
{
if (model == NULL) {
return false;
}

const bool is_sim = model != NULL && model->get_simulation();
const float mouth_angle_threshold = is_sim ? SIM_SHOOTPING_MOUTH_ANGLE_THRESHOLD : REAL_SHOOTPING_MOUTH_ANGLE_THRESHOLD;

// 获取球员坐标
const point2f& player_pos = model->get_our_player_pos(robot_id);

// 获取球的位置
const point2f& ball_pos = model->get_ball_pos();

// 获取球员朝向
const float my_dir = model->get_our_player_dir(robot_id);

// 小车到球的距离
const float ball_dist = (ball_pos - player_pos).length();

// 车头方向和本帧射门方向的夹角。
// 控球判断和最终射门使用同一个方向，避免机器人已经对准空当，
// 却因为没有对准球门中心而一直无法触发踢球。
const float dir_error = fabs(Maths::normalizeAngle(shoot_dir - my_dir));

// 距离阈值：球离小车中心多近，算在控球嘴附近
// get_ball_threshold = 18.0f;

// 角度阈值：车头必须对准球门


// 判断球是否离小车足够近
const bool ball_near = ball_dist < get_ball_threshold;

// 判断车头是否朝向选定的射门目标。
const bool facing_goal = dir_error < mouth_angle_threshold;

return ball_near && facing_goal;
}


//==================== 功能块 2：主函数初始化 ====================
// 创建任务对象，判断世界模型是否有效。
PlayerTask player_plan(const WorldModel* model, int robot_id)
{
PlayerTask task;

if (model == NULL) {
return task;
}

const bool is_sim = model != NULL && model->get_simulation();
const float get_ball_back_dist = is_sim ? SIM_SHOOTPING_GET_BALL_BACK_DIST : REAL_SHOOTPING_GET_BALL_BACK_DIST;
const float ready_back_dist = is_sim ? SIM_SHOOTPING_READY_BACK_DIST : REAL_SHOOTPING_READY_BACK_DIST;
const float approach_back_dist = is_sim ? SIM_SHOOTPING_APPROACH_BACK_DIST : REAL_SHOOTPING_APPROACH_BACK_DIST;
const double kick_power = is_sim ? SIM_SHOOTPING_KICK_POWER : REAL_SHOOTPING_KICK_POWER;
const float post_margin = is_sim ? SIM_SHOOTPING_POST_MARGIN : REAL_SHOOTPING_POST_MARGIN;

//==================== 功能块 3：获取场上关键信息 ====================
// 获取当前小车和球的位置，后面据此计算拿球点和射门方向。

// 获取球的位置
const point2f& ball_pos = model->get_ball_pos();

//==================== 功能块 4：根据对方守门员位置选择射门方向 ====================
// 在上下门柱内侧保留安全边距，再比较守门员上下两侧的有效空当。
// 最终瞄准较大空当的中心，而不是直接瞄准门柱。
const float safe_top_y = static_cast<float>(GOAL_WIDTH / 2) - post_margin;
const float safe_bottom_y = static_cast<float>(-GOAL_WIDTH / 2) + post_margin;
point2f shoot_target;
int opp_goalie_id = model->get_opp_goalie();
bool goalie_valid = (opp_goalie_id >= 0 && opp_goalie_id < 6 && model->get_opp_exist_id()[opp_goalie_id]);

if (goalie_valid)
{
	// 将守门员 Y 坐标限制在有效门框内，防止守门员出击时产生越界目标。
	const point2f& opp_goalie_pos = model->get_opp_player_pos(opp_goalie_id);
	float goalie_y = opp_goalie_pos.y;
	if (goalie_y > safe_top_y)
	{
		goalie_y = safe_top_y;
	}
	else if (goalie_y < safe_bottom_y)
	{
		goalie_y = safe_bottom_y;
	}

	const float upper_gap = safe_top_y - goalie_y;
	const float lower_gap = goalie_y - safe_bottom_y;

	if (upper_gap > lower_gap)
	{
		// 守门员上方空当更大：瞄准上方空当中心。
		shoot_target = point2f(
			static_cast<float>(FIELD_LENGTH_H),
			(safe_top_y + goalie_y) * 0.5f
		);
	}
	else
	{
		// 守门员下方空当更大：瞄准下方空当中心。
		shoot_target = point2f(
			static_cast<float>(FIELD_LENGTH_H),
			(safe_bottom_y + goalie_y) * 0.5f
		);
	}
}
else
{
	// 没有可靠守门员信息时，根据球的位置选择球门另一侧的安全目标。
	if (ball_pos.y > 0)
	{
		shoot_target = point2f(
			static_cast<float>(FIELD_LENGTH_H), safe_bottom_y
		);
	}
	else
	{
		shoot_target = point2f(
			static_cast<float>(FIELD_LENGTH_H), safe_top_y
		);
	}
}

float face_dir = (shoot_target - ball_pos).angle();


//==================== 功能块 5：设置默认拿球任务 ====================
// 默认先去球后方，打开吸球，不踢球。
// 如果还没有控到球，小车会执行这个默认拿球动作。

// 默认：沿选定射门方向移动到球后方。
task.orientate = face_dir;
task.target_pos = ball_pos - Maths::vector2polar(get_ball_back_dist, face_dir);

// 默认打开吸球
task.needCb = true;

// 默认先不踢
task.needKick = false;
task.isPass = false;


//==================== 功能块 6：控到球后的踢球逻辑 ====================
// 如果判断球已经在控球嘴上，
// 就保持朝向球门方向，开启击球，并设置踢球力度。
if (isget(model, robot_id, face_dir))
{
task.target_pos = ball_pos - Maths::vector2polar(ready_back_dist, face_dir);


task.orientate = face_dir;

// 停在当前位置，
//task.target_pos = player_pos;

// 打开吸球
task.needCb = true;

// 平射传球，不是挑射
task.isChipKick = false;

// 开启击球
task.needKick = true;

// 这是射门，不是传球。
task.isPass = false;

// 平射力度。
task.kickPower = kick_power;
}


//==================== 功能块 7：未控到球时继续拿球 ====================
// 如果球还没有进入控球嘴，
// 小车继续移动到球后方，并保持吸球开启。
else
{
// 如果球还没到控球嘴，就去球后方拿球
task.orientate = face_dir;

// 这里的 8 可以调：
// 小一点更贴球，大一点更保守
task.target_pos = ball_pos - Maths::vector2polar(approach_back_dist, face_dir);

// 拿球过程中也打开吸球
task.needCb = true;

task.needKick = false;
task.isPass = false;
}


//==================== 功能块 8：返回任务 ====================
// 把本帧计算出的目标点、朝向、吸球、踢球等任务返回给系统执行。
return task;
}
#endif
