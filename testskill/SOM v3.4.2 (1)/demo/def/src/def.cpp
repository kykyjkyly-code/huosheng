#include "def.h"
#include "utils/maths.h"
//用户注意；接口需要如下声明
extern "C"_declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);



static bool moveToBall = false;
//防止变量之前的变量来处理这个内容
static point2f ballPosRec = { 9999, 9999 };
static bool isGetBall = false;
float normalizeAngle(float angle)
{
	
	while (angle > PI) angle -= 2 * PI;
	while (angle < -PI) angle += 2 * PI;
	return angle;
}
PlayerTask player_plan(const WorldModel* model, int robot_id){
	PlayerTask task;
	const point2f& goal = FieldPoint::Goal_Center_Point;

	//球员坐标
	const point2f& playerPos = model->get_our_player_pos(robot_id);
	//球坐标
	const point2f& ball = model->get_ball_pos();
	//放置点坐标
	const point2f& placePos = ball - Maths::vector2polar(5 + MAX_ROBOT_SIZE, 10);
	//当前车头朝向
	float robotDir = model->get_our_player_dir(robot_id);
	//一些距离还有我设置的方向
	float dirDiff = fabs(normalizeAngle(10 - robotDir));

	float toBallDist = (playerPos - ball).length();
	float toBallDir = (ball - playerPos).angle();

	bool getBall = toBallDist < 10;
	float diffdir_onorbit = 0;
	//float b2r = BallToRobot.angle();

	bool add;
	//有圆圈大小和角度值
	const float& circleR = 10;
	const float& DetAngle = 0.6;
	task.needCb = false;


	if ((ball - playerPos).length() > circleR + MAX_ROBOT_SIZE)										//距离未到达足够近, 车向球靠近
	{
		//车跑向球
		task.orientate = (ball - playerPos).angle();
		task.target_pos = ball + Maths::vector2polar(5 + MAX_ROBOT_SIZE, task.orientate);

		if (ball.X() < 300 && ball.X() > -300 && ball.Y() < 200 && ball.Y() > -200)			//球在图像中时，保存球的位置
		{
			moveToBall = true;
			ballPosRec = ball;
		}	
	}
	else
	{
		//车头绕着球转到朝向放置点
		point2f BallToRobot = playerPos - ballPosRec;

		float b2r = BallToRobot.angle();
		float o2b = (ballPosRec - placePos).angle();

		if (b2r * o2b > 0){
			if (b2r > 0){
				if (b2r > o2b)
					add = false;
				else
					add = true;
			}
			else{
				if (b2r > o2b)
					add = false;
				else
					add = true;
			}
		}
		else{
			if (b2r > 0)
				add = true;
			else
				add = false;
		}
		if (add)
		{
			//+
			task.target_pos = ball + Maths::vector2polar(circleR, BallToRobot.angle() + DetAngle);
			task.orientate = (placePos - ball).angle();
		}
		else
		{
			//-
			task.target_pos = ball + Maths::vector2polar(circleR, BallToRobot.angle() - DetAngle);
			task.orientate = (placePos - ball).angle();
		}
		//吸球
	}
	cout << "ballPosRec: " << ballPosRec << endl;
	cout << "playerPos: " << playerPos<<endl;
	cout << "length: " << (ballPosRec - playerPos).length() << endl;

	/*if ((ballPosRec - playerPos).length() < 5  && fabs((placePos - playerPos).angle() - (placePos - ballPosRec).angle()) < 0.1 )
	{
		//车已拿到球
		isGetBall = true;
		cout << "Enter ***GetBall*** state" << endl;
	}

	if (isGetBall)
	{
		task.needCb = true;
		task.flag = true;
// 		point2f playerVel = model->get_our_player_v(robot_id);
// 		if (playerVel.length() > 70)
// 		{
// 			
// 		}
		if ((placePos - playerPos).length() < 10)	//到达放置点
		{
			task.target_pos = playerPos;
			task.orientate = model->get_our_player_dir(robot_id);
			task.needCb = false;
			isGetBall = false;
		}
		else
		{
			task.target_pos = placePos;
			task.orientate = (placePos - playerPos).angle();
		}
		cout << "Enter move to place pos" << endl;
	}
	cout << "[needcb] :" << task.needCb << endl;
	cout << "----------------------------------------------" << endl;
	//cout<<"posX: "<<placePos.x<<",  posY:"<< placePos.y;
	*/
	return task;
}