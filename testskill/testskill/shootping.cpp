#if 0
#include"src\utils\PlayerTask.h"
#include"src\getballsource.h"
#include"src\utils\worldmodel.h"
#include"src\utils\maths.h"
#include <cmath>

/*
功能流程：
1. 本文件负责平射：先找队友/球门方向拿球，控到球后执行平射。
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


//==================== 功能块 1：判断是否控到球 ====================
// 根据球和小车的位置、距离、方向，判断球是否已经在控球嘴附近。
bool isget(const WorldModel* model, int robot_id)
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

// 小车指向球门中心的方向
const point2f goal_center(static_cast<float>(FIELD_LENGTH_H), 0.0f);
const float goal_dir = (goal_center - player_pos).angle();

// 车头方向和球门方向的夹角
const float dir_error = fabs(Maths::normalizeAngle(goal_dir - my_dir));

// 距离阈值：球离小车中心多近，算在控球嘴附近
// get_ball_threshold = 18.0f;

// 角度阈值：车头必须对准球门


// 判断球是否离小车足够近
const bool ball_near = ball_dist < get_ball_threshold;

// 判断车头是否朝向球门
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


//==================== 功能块 3：寻找接球队友 ====================
// 从我方机器人中选择一台非自己、非守门员的车作为接球队员。
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


//==================== 功能块 4：没有接球队友时的处理 ====================
// 如果没有找到接球队员，就直接返回空任务。
// 小车不会主动执行拿球、传球或射门。
if (receiver_id == -1)
{
return task;
}


//==================== 功能块 5：获取场上关键信息 ====================
// 获取当前小车、球、接球队友、球门的位置。
// 后面根据这些信息计算拿球点和踢球方向。

// 获取传球队员坐标
const point2f& player_pos = model->get_our_player_pos(robot_id);

// 获取球的位置
const point2f& ball_pos = model->get_ball_pos();

// 获取接球队员的位置
const point2f& receiver_pos = model->get_our_player_pos(receiver_id);

// 敌方球门中心
point2f goal = -FieldPoint::Goal_Center_Point;


//==================== 功能块 6：判断球在球门哪一侧，射向另一端 ====================
// 根据球在场上的 Y 坐标判断球在球门上方还是下方，
// 射向球门的相反一侧。如果球在原点附近（无球），默认射中间。
const point2f goal_top(static_cast<float>(FIELD_LENGTH_H), static_cast<float>(GOAL_WIDTH / 2));
const point2f goal_bottom(static_cast<float>(FIELD_LENGTH_H), static_cast<float>(-GOAL_WIDTH / 2));
const point2f goal_center(static_cast<float>(FIELD_LENGTH_H), 0.0f);

point2f shoot_target;
if (ball_pos.length() < 1.0f)
{
	// 没有球，默认射球门中间
	shoot_target = goal_center;
}
else if (ball_pos.y > 0)
{
	// 球在球门上方 → 射向球门下角
	shoot_target = goal_bottom;
}
else
{
	// 球在球门下方 → 射向球门上角
	shoot_target = goal_top;
}

float face_dir = (shoot_target - ball_pos).angle();


//==================== 功能块 7：设置默认拿球任务 ====================
// 默认先去球后方，打开吸球，不踢球。
// 如果还没有控到球，小车会执行这个默认拿球动作。

// 默认：去球的后方，车头朝向接球队员
task.orientate = face_dir;
task.target_pos = ball_pos - Maths::vector2polar(get_ball_back_dist, face_dir);

// 默认打开吸球
task.needCb = true;

// 默认先不踢
task.needKick = false;
task.isPass = false;


//==================== 功能块 8：控到球后的踢球逻辑 ====================
// 如果判断球已经在控球嘴上，
// 就保持朝向球门方向，开启击球，并设置踢球力度。
if (isget(model, robot_id))
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

// 标记这是传球
task.isPass = true;

// 传球力度，可以根据距离调整
task.kickPower = kick_power;
}


//==================== 功能块 9：未控到球时继续拿球 ====================
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


//==================== 功能块 10：返回任务 ====================
// 把本帧计算出的目标点、朝向、吸球、踢球等任务返回给系统执行。
return task;
}
#endif
