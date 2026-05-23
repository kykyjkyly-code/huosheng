#if 0
#include "src\utils\PlayerTask.h"
#include "src\getballsource.h"
#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"


/*
�������̣�
1. ���ļ���������/����ǰ���������ȿ����������Ƶ�Ŀ�귽���󷽣�����������������
2. ����ʱͨ�� model->get_simulation() �жϵ�ǰ�Ƿ��滹��ʵ�ء�
3. ��Ҫ������ʱֻ�������� REAL_* �� SIM_*��player_plan �����Զ�ѡ����Ӧ������
*/

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== PGET������ ====================*/
// û�ж���ʱ��վ�����󷽵Ķ������롣
const float REAL_PGET_NO_RECEIVER_BACK_EXTRA = 5.0f;
const float SIM_PGET_NO_RECEIVER_BACK_EXTRA = 5.0f;
// �̶�Ŀ���㣬���� jie.cpp �Ľ����㡣
const float REAL_PGET_TARGET_POS_X = 20.0f;
const float REAL_PGET_TARGET_POS_Y = 70.0f;
const float SIM_PGET_TARGET_POS_X = 20.0f;
const float SIM_PGET_TARGET_POS_Y = 70.0f;
// �����뾶�ڻ����˰뾶�����϶����ӵľ��롣
const float REAL_PGET_CIRCLE_EXTRA_DIST = 10.0f;
const float SIM_PGET_CIRCLE_EXTRA_DIST = 10.0f;
// ��������Բ�ܸ������ݲ
const float REAL_PGET_ARRIVE_CIRCLE_ERR = 4.0f;
const float SIM_PGET_ARRIVE_CIRCLE_ERR = 4.0f;
// ÿһ֡���������ĽǶȲ�����
const float REAL_PGET_ORBIT_STEP_ANGLE = 0.25f;
const float SIM_PGET_ORBIT_STEP_ANGLE = 0.30f;
// ��Ϊ�Ѿ��Ƶ����ʽǶȵ�������ֵ��ԽСԽ��ȷ����
const float REAL_PGET_ALIGN_ANGLE = 0.15f;
const float SIM_PGET_ALIGN_ANGLE = 0.15f;
// С������̫Զʱ����״̬���Ķ������롣
const float REAL_PGET_RESET_FAR_EXTRA = 25.0f;
const float SIM_PGET_RESET_FAR_EXTRA = 25.0f;


/*==================== ���ܿ� 3������״̬���� ====================*/
/*
GO_TO_BALL���ȿ�����
ORBIT_BALL����������λ�ã�����΢�����ֺ���λ��
*/
enum RotateBallState
{
	GO_TO_BALL = 0,     // �����������ܵ��򸽽�
	ORBIT_BALL = 1     // ���򣬳�ͷһֱָ���򣬳���΢��
};


/*==================== ���ܿ� 4��״̬���� ====================*/
/*
state ��¼��ǰִ�е���һ����
*/
static RotateBallState state = GO_TO_BALL;


PlayerTask player_plan(const WorldModel* model, int robot_id)
{
	/*==================== ���ܿ� 5���������񲢶�ȡ������Ϣ ====================*/
	/*
	��ȡ��ǰС��λ�á�С��������С��λ�á�
	��������Ŀ�����ͷ��򶼻�����Щ��Ϣ���㡣
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


	/*==================== ���ܿ� 6��Ѱ�ҽ������� ====================*/
	/*
	���ҷ���������Ѱ��һ�����Լ���������Ա�Ķ��ѣ�
	��Ϊ������������Ŀ�ꡣ
	*/
	// ��һ���������ѡ�������Ȼ��������д����ѡ��������һ�����Լ���������Ա���ҷ�����
	int receiver_id = -1;

	for (int i = 0; i < 6; i++)
	{
		if (i == robot_id || i == model->get_our_goalie())
			continue;

		if (model->get_our_exist_id()[i])
			receiver_id = i;
	}


	/*==================== ���ܿ� 7��û�н�������ʱ�Ĵ��� ====================*/
	/*
	����û���ҵ��������ѣ�
	С�����ƶ����򸽽��������ֳ�ͷ��������
	*/
	// û�н�����Աʱ���ȵ����󷽣�������
	if (receiver_id == -1)
	{
		float faceBallDir = (ball_pos - player_pos).angle();
		task.orientate = faceBallDir;
		task.target_pos = ball_pos - Maths::vector2polar(no_receiver_back_extra + MAX_ROBOT_SIZE, faceBallDir);
		task.needCb = true;
		return task;
	}


	/*==================== ���ܿ� 8�����㴫������������Ŀ�귽�� ====================*/
	/*
	��������Ϊ����ָ��������Ա��ͷ��������λ�ã���
	С������Ӧ���Ƶ����ķ�������
	�γɡ�С�� ���� �� ���� ������Ա��ͷ����λ�ù�ϵ��
	*/
	const point2f& receiverPos = model->get_our_player_pos(receiver_id);
	const float receiverDir = model->get_our_player_dir(receiver_id);
	const point2f receiverHeadPos = receiverPos + Maths::vector2polar(static_cast<float>(ROBOT_HEAD), receiverDir);

	// �� -> ������Ա��ͷ������Ҳ���� kicker ����Ҫ����/�����ķ���
	float passDir = (receiverHeadPos - ball_pos).angle();

	// ������ʱ������Ӧ��վ�����ķ�������
	// ������Ա ---- �� ---- С��
	float targetRobotRelDir = Maths::normalizeAngle(passDir + static_cast<float>(PI));

	// С����ǰ�������ķ������� -> С��
	float currentRobotRelDir = (player_pos - ball_pos).angle();

	// �������ľ���
	float distToBall = (player_pos - ball_pos).length();


	/*==================== ���ܿ� 9���������� ====================*/
	/*
	��Щ�������������뾶���Ƕ��жϡ������ٶȺ���С�������롣
	��Ҫ���ڵ���С���Ƿ��Ƶ��ȡ������Ƿ�̫�졣
	*/
	// �������԰�����Ч��΢��
	const float circleR = circle_extra_dist + MAX_ROBOT_SIZE;     // �����뾶����ʾ���ĵ������ĵľ���
	const float arriveCircleErr = arrive_circle_err_value;               // ����Բ�ܸ������ݲ�
	const float DetAngle = orbit_step_angle;


	//�������޸ĽǶ�����


	// ÿ�������Ƕȣ�ԽСԽƽ����Խ��Խ��
	const float alignAngle = align_angle_value;                   // �Ƕȶ�����ֵ��ԽСԽ��ȷ

	task.needCb = true;   // ������/����

	/*==================== ���ܿ� 10���쳣�������� ====================*/
	/*
	����С������̫Զ��
	˵����ǰ����״̬�����Ѿ�ʧЧ��
	���»ص� GO_TO_BALL �׶Ρ�
	*/
	// �������ͳ�ͻȻ����̫Զ�����½��������׶�
	if (distToBall > circleR + reset_far_extra)
	{
		state = GO_TO_BALL;
	}


	/*==================== ���ܿ� 11��״̬������ ====================*/
	/*
	���ݵ�ǰ״̬������С��ִ�У�
	�����������򡢻�����������������
	*/
	switch (state)
	{
	case GO_TO_BALL:
	{
		/*==================== ״̬ 1�������� ====================*/
		/*
		С���������򣬲��ƶ����򸽽���Բ��λ�á�
		����ָ�������󣬽�������״̬��
		*/
		// һ��ʼС��������ȥ����
		float faceBallDir = (ball_pos - player_pos).angle();
		task.orientate = faceBallDir;

		// ��ֱ�ӳ嵽���ģ�����ͣ����ǰ��һ������
		task.target_pos = ball_pos - Maths::vector2polar(circleR, faceBallDir);

		if (distToBall <= circleR + arriveCircleErr)
		{
			state = ORBIT_BALL;
		}

		break;
	}

	case ORBIT_BALL:
	{
		/*==================== ״̬ 2�����������Ƕ� ====================*/
		/*
		С����������ΪԲ�ĵ�Բ�˶���
		Ŀ�����Ƶ����ĺ󷽣�
		�á�С�� ���� �� ���� �������ѡ����롣
		*/
		// ���������У���ͷʼ��ָ����
		task.orientate = (ball_pos - player_pos).angle();

		// �жϵ�ǰ����->�����ǶȺ�Ŀ�ꡰ��->�����Ƕ��Ƿ��ӽ�
		float err = Maths::normalizeAngle(targetRobotRelDir - currentRobotRelDir);

		if (fabs(err) < alignAngle)
		{
			// �Ƕ��Ѷ��룬������Ŀ��λ�ó���΢��
			task.target_pos = ball_pos + Maths::vector2polar(circleR, targetRobotRelDir);
			break;
		}

		// �����̷�������
		float step = 0.0f;

		if (err > 0)
			step = DetAngle;
		else
			step = -DetAngle;

		// ��ֹ����һ���ƹ�ͷ
		if (fabs(err) < DetAngle)
			step = err;

		float nextRelDir = Maths::normalizeAngle(currentRobotRelDir + step);

		// С��Ŀ������������ΪԲ�ĵ�Բ��
		task.target_pos = ball_pos + Maths::vector2polar(circleR, nextRelDir);

		break;
	}

	default:
	{
		/*==================== ״̬�쳣���� ====================*/
		/*
		����״ֵ̬�쳣�����»ص� GO_TO_BALL��
		*/
		state = GO_TO_BALL;
		break;
	}
	}

	return task;
}
#endif
