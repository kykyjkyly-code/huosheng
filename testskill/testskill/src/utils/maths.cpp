
/*
�������̣�
1. ���ļ��ṩ��ѧ���ߺͳ��عؼ��㶨�壬��ֱ��ִ�л����˲��ԡ�
2. ����û�� WorldModel����˲��ڱ��ļ��жϷ����ʵ�ء�
3. ����Ժ������Ҫ���ε���ѧ�߼������ڵ��÷����� model->get_simulation() ѡ��������ٴ��롣
*/

#include "maths.h"
namespace FieldPoint{
	point2f Goal_Center_Point(static_cast<float>(-FIELD_LENGTH / 2), 0.0f);
	point2f Penalty_Kick_Point(static_cast<float>(-FIELD_LENGTH / 2 + PENALTY_KICKER_L), 0.0f);
	point2f Goal_Left_Point(static_cast<float>(-FIELD_LENGTH_H), static_cast<float>(-GOAL_WIDTH_H));
	point2f Goal_Right_Point(static_cast<float>(-FIELD_LENGTH_H), static_cast<float>(GOAL_WIDTH_H));
	point2f Goal_Center_Left_One_Point(static_cast<float>(-FIELD_LENGTH / 2), static_cast<float>(-PENALTY_BISECTOR));
	point2f Goal_Center_Left_Two_Point(static_cast<float>(-FIELD_LENGTH / 2), static_cast<float>(-PENALTY_BISECTOR * 2));
	point2f Goal_Center_Right_One_Point(static_cast<float>(-FIELD_LENGTH / 2), static_cast<float>(PENALTY_BISECTOR));
	point2f Goal_Center_Right_Two_Point(static_cast<float>(-FIELD_LENGTH / 2), static_cast<float>(PENALTY_BISECTOR * 2));
	point2f Goal_Penalty_Area_L_Right(static_cast<float>(-FIELD_LENGTH), static_cast<float>(PENALTY_AREA_L / 2));
	point2f Goal_Penalty_Area_L_Left(static_cast<float>(-FIELD_LENGTH), static_cast<float>(-PENALTY_AREA_L / 2));
	point2f Penalty_Area_L_A(static_cast<float>(PENALTY_AREA_R - FIELD_LENGTH_H), static_cast<float>(PENALTY_AREA_L / 2));
	point2f Penalty_Area_L_B(static_cast<float>(PENALTY_AREA_R - FIELD_LENGTH_H), static_cast<float>(-PENALTY_AREA_L / 2));
	point2f Penalty_Arc_Center_Right(static_cast<float>(-FIELD_LENGTH_H), static_cast<float>(PENALTY_AREA_L/2));
	point2f Penalty_Arc_Center_Left(static_cast<float>(-FIELD_LENGTH_H), static_cast<float>(-PENALTY_AREA_L/2));
	point2f Penalty_Rectangle_Left(static_cast<float>(-FIELD_LENGTH_H + PENALTY_AREA_R), static_cast<float>(-PENALTY_AREA_L / 2));
	point2f Penalty_Rectangle_Right(static_cast<float>(-FIELD_LENGTH_H + PENALTY_AREA_R), static_cast<float>(PENALTY_AREA_L / 2));
}
 namespace Maths{
	  float normalize(float theta)
	 {
		 if (theta >= -PI && theta < PI)
			 return theta;
		 int multiplier = (int)(theta / (2 * PI));
		 theta = static_cast<float>(theta - multiplier * 2 * PI);
		 if (theta >= PI)
			 theta -= static_cast<float>(2 * PI);
		 if (theta < -PI)
			 theta += static_cast<float>(2 * PI);
		 return theta;
	 }

	 inline point2f vector2polar(float length, float dir){
		 return point2f(length*cos(dir), length*sin(dir));
	 }

	 point2f circle_segment_intersection(const point2f& start_point, const double circle_r, const  point2f& end_point){
		 float orientation_ang = (end_point - start_point).angle();
		 return start_point + vector2polar(static_cast<float>(circle_r), orientation_ang);
	 }

	 point2f archimedes_spiral(const point2f& spiral_center, float spiral_center_size, const point2f& pos, float spiral_buff){
		 point2f spiral_point;
		 float dist = (pos - spiral_center).length();
		 float angle = (pos - spiral_center).angle();
		 float dist_step = 8.0f;           //������ת180��5����ת�ɹ�����Ӧdist_step Ϊ40/5-- fuck ִ�к�˼·�е�ƫ�� �Ȳ���
		 float ang_step = static_cast<float>(PI / 180 * 36);
		 float ang_deta = Maths::normalize(angle - ang_step);
		 float dist_deta = dist - dist_step;
		 if (dist_deta < spiral_center_size + spiral_buff) dist_deta = spiral_center_size + spiral_buff;
		 spiral_point = spiral_center + Maths::vector2polar(dist_deta,ang_deta);
		 return spiral_point;
	 }

	  int sign(float d){ return (d >= 0) ? 1 : -1; }

	  float vector_angle(const point2f& a, const point2f& b, const point2f& c){
		  return  (angle_mod((a - b).angle() - (c - b).angle()));
	  }

	  float least_squares(const vector<point2f>& points){
		  float x_mean = 0;
		  float y_mean = 0;
		  float Dxx = 0, Dxy = 0, Dyy = 0;
		  float a = 0, b = 0;
		  for (std::vector<point2f>::size_type i = 0; i < points.size(); i++)
		  {
			  x_mean += points[i].x;
			  y_mean += points[i].y;
		  }
		  x_mean /= points.size();
		  y_mean /= points.size(); //���ˣ�������� x y �ľ�ֵ

		  
		  for (std::vector<point2f>::size_type i = 0; i < points.size(); i++)
		  {
			  Dxx += (points[i].x - x_mean) * (points[i].x - x_mean);
			  Dxy += (points[i].x - x_mean) * (points[i].y - y_mean);
			  Dyy += (points[i].y - y_mean) * (points[i].y - y_mean);
		  }

		  double lambda = ((Dxx + Dyy) - sqrt((Dxx - Dyy) * (Dxx - Dyy) + 4 * Dxy * Dxy)) / 2.0;
		  double den = sqrt(Dxy * Dxy + (lambda - Dxx) * (lambda - Dxx));
		  
		  if (fabs(den) < 1e-5)
		  {
			  return 0.0;
		  }
		  else
		  {
			  a = static_cast<float>(Dxy / den);
			  b = static_cast<float>((lambda - Dxx) / den);
			  return static_cast<float>(atan(-a/b));
		  }
	  }

	  point2f line_perp_across(const point2f& p1, float slope, const point2f& p2){
		  float a, b, b1;
		  if (isnan(slope)){
			  a = 1;
			  b = 0;
		  }
		  else if (fabs(slope) < 0.00001){
			  a = 0;
			  b = 1;
		  }
		  else
		  {
			  a = slope;
			  b = p1.y - a*p1.x;
		  }
		  b1 = -p2.x - a*p2.y;
		  point2f across_point;
		  across_point.x = (-b1 - a*b) / (a*a + 1);
		  across_point.y = a * across_point.x + b;
		  return across_point;
	  }

	  bool  in_range(const point2f& p1, const point2f& left_down, const point2f& right_up){
		  return (p1.x < right_up.x && p1.x > left_down.x) && (p1.y < right_up.y && p1.y > left_down.y);
	  }

	  point2f across_point(const point2f &p1, const point2f &p2, const point2f &q1, const point2f &q2){
		point2f project_point_a = point_on_segment(p1,p2,q1,true);
		point2f project_point_b = point_on_segment(p1,p2,q2,true);
		float a = (q1 - project_point_a).length();
		float b = (q2 - project_point_b).length();
		return (project_point_b*(a / (a + b)) + project_point_a*(b / (a + b)));
	}
			
	  bool is_inside_penatly(const point2f& ball){
		  if (fabs(ball.x) > FIELD_LENGTH_H - PENALTY_AREA_R  && ball.x < 0){
			  if (fabs(ball.y) < PENALTY_AREA_L / 2)
				  return true;
			  else if (ball.y < 0 && (ball - FieldPoint::Goal_Penalty_Area_L_Left).length() < PENALTY_AREA_R)
				  return true;
			  else if (ball.y > 0 && (ball - FieldPoint::Goal_Penalty_Area_L_Right).length() < PENALTY_AREA_R)
				  return true;
			  else
				  return false;
		  }
		  return false;
	  }
}

