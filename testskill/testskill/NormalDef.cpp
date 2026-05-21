#if 0
#include "NormalDef.h"
#include"src\utils\PlayerTask.h"
#include"src\getballsource.h"
#include"src\utils\worldmodel.h"
#include"src\utils\maths.h"
#include <cmath>

/*
功能流程：
1. 本文件负责普通防守：根据球的位置选择禁区圆弧或矩形防守点。
2. 运行时通过 model->get_simulation() 判断当前是仿真还是实地。
3. 需要调参数时只改下面的 REAL_* 或 SIM_*，player_plan 里会自动选择对应参数。
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 普通防守调参区 ====================*/
// 圆弧区域防守点离禁区边的比例。
const float REAL_NORMALDEF_ARC_EXTRA_RATIO = 0.5f;
const float SIM_NORMALDEF_ARC_EXTRA_RATIO = 0.5f;
// 防守目标点平滑系数，越大越稳但越慢。
const float REAL_NORMALDEF_SMOOTH = 0.85f;
const float SIM_NORMALDEF_SMOOTH = 0.85f;

enum PenaltyArea
{
	RightArc,
	MiddleRectangle,
	LeftArc
};
NormalDef::NormalDef()
{
}

NormalDef::~NormalDef()
{
}

PlayerTask player_plan(const WorldModel* model, int robot_id){
	//创建PlayerTask对象task，task对象是一个任务方法集合
	PlayerTask task;
	if (model == NULL) {
		return task;
	}

	const bool is_sim = model != NULL && model->get_simulation();
	const float arc_extra_ratio = is_sim ? SIM_NORMALDEF_ARC_EXTRA_RATIO : REAL_NORMALDEF_ARC_EXTRA_RATIO;
	const float normaldef_smooth = is_sim ? SIM_NORMALDEF_SMOOTH : REAL_NORMALDEF_SMOOTH;

	//以下为执行防守需要的参数，部分参数解释可以参看GetBall.cpp
	const point2f& goal = FieldPoint::Goal_Center_Point;
	const point2f& arc_center_right = FieldPoint::Penalty_Arc_Center_Right;
	const point2f& arc_center_left = FieldPoint::Penalty_Arc_Center_Left;
	const point2f& rectangle_left = FieldPoint::Penalty_Rectangle_Left;
	const point2f& rectangle_right = FieldPoint::Penalty_Rectangle_Right;
	const point2f& ball = model->get_ball_pos();
	//area为枚举变量，根据不同的ball位置，设置不同的枚举值
	PenaltyArea area;
	if (ball.y > arc_center_right.y)
		area = RightArc;
	else if (ball.y < arc_center_left.y)
		area = LeftArc;
	else
		area = MiddleRectangle;
	//switch根据不同的area值，设置不同的task
	switch (area)
	{
	case RightArc:
		//任务小车的朝向角及目标点
		task.orientate = (ball - goal).angle();
		task.target_pos = goal + Maths::vector2polar(PENALTY_AREA_R + MAX_ROBOT_SIZE + PENALTY_AREA_R * arc_extra_ratio, task.orientate);
		break;
	case MiddleRectangle:
		task.orientate = (ball - goal).angle();
		//across_point(p1, p2, p3, p4)函数是求p1p2线段和p3p4线段的交点
		task.target_pos = Maths::across_point(rectangle_left,rectangle_right,ball,goal);
		break;
	case LeftArc:
		task.orientate = (ball - goal).angle();
		task.target_pos = goal + Maths::vector2polar(PENALTY_AREA_R + MAX_ROBOT_SIZE + PENALTY_AREA_R * arc_extra_ratio, task.orientate);
		break;
	default:
		break;
	}
	

	/*====================【功能块：让防守变呆滞】====================*/
	static bool has_last_task = false;
	static point2f last_target_pos;
	static float last_orientate = 0.0f;
	//就是改变smooth来判断这个的尺钝性
	const float smooth = normaldef_smooth;

	if (!has_last_task)
	{
		last_target_pos = task.target_pos;
		last_orientate = task.orientate;
		has_last_task = true;
	}
	else
	{
		last_target_pos.x = last_target_pos.x * smooth + task.target_pos.x * (1.0f - smooth);
		last_target_pos.y = last_target_pos.y * smooth + task.target_pos.y * (1.0f - smooth);

		last_orientate = last_orientate * smooth + task.orientate * (1.0f - smooth);

		task.target_pos = last_target_pos;
		task.orientate = last_orientate;
	}

	
	return task;
}
#endif
