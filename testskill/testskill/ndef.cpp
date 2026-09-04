#if 0

#include <iostream>
#include <cstring>
#ifndef EPSILON
#define EPSILON (1.0E-10)
#endif

#include "src\utils\PlayerTask.h"
#include "src\utils\worldmodel.h"
#include "src\utils\maths.h"
#include <cmath>

/*
 * 3V3 整队防守技能（守门员 + 两名后卫）。
 *
 * 使用方法：
 * 1. 将本文件第一行改成 #if 1。
 * 2. 将工程内其他导出 player_plan 的技能改成 #if 0，保证 DLL 只有一个入口。
 * 3. 三台我方车都可以调用同一个 DLL；代码会根据 get_our_goalie() 自动分工。
 *
 * 后卫 1：沿用官方 NormalDef 的思路，封堵“球 -> 我方球门”的主射门线。
 * 后卫 2：封堵第二危险进攻队员；没有第二威胁时，覆盖球的另一侧。
 * 守门员：留在门线上方的小范围内，跟随球的射门角度移动。
 */

extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);

/*==================== 3V3 防守调参区 ====================*/

// 后卫站在官方禁区边界外的额外安全距离（厘米）。
const float REAL_DEF3_PENALTY_BUFFER = 12.0f;
const float SIM_DEF3_PENALTY_BUFFER = 10.0f;

// 没有第二名进攻威胁时，两名后卫在防线上的角度间隔。
const float REAL_DEF3_COVER_ANGLE = 0.32f;
const float SIM_DEF3_COVER_ANGLE = 0.28f;

// 两名后卫目标点允许的最小距离。
const float REAL_DEF3_TEAMMATE_GAP = 25.0f;
const float SIM_DEF3_TEAMMATE_GAP = 23.0f;

// 后卫目标平滑：越大越稳，越小反应越快。
const float REAL_DEF3_SMOOTH = 0.72f;
const float SIM_DEF3_SMOOTH = 0.62f;

// 守门员离球门中心的站位半径，以及左右活动范围。
const float REAL_DEF3_GOALIE_DEPTH = 16.0f;
const float SIM_DEF3_GOALIE_DEPTH = 18.0f;
const float REAL_DEF3_GOALIE_Y_LIMIT = 27.0f;
const float SIM_DEF3_GOALIE_Y_LIMIT = 27.0f;

// 目标点离场地边线至少保留的距离。
const float DEF3_FIELD_MARGIN = static_cast<float>(MAX_ROBOT_SIZE + 2);

namespace
{
    float clampf(float value, float low, float high)
    {
        if (value < low) return low;
        if (value > high) return high;
        return value;
    }

    bool valid_team_id(int id)
    {
        return id >= 0 && id < MAX_TEAM_ROBOTS;
    }

    point2f clamp_to_field(const point2f& p)
    {
        return point2f(
            clampf(p.x,
                static_cast<float>(-FIELD_LENGTH_H + DEF3_FIELD_MARGIN),
                static_cast<float>(FIELD_LENGTH_H - DEF3_FIELD_MARGIN)),
            clampf(p.y,
                static_cast<float>(-FIELD_WIDTH_H + DEF3_FIELD_MARGIN),
                static_cast<float>(FIELD_WIDTH_H - DEF3_FIELD_MARGIN))
        );
    }

    /*
     * 根据官方 NormalDef 的三区域逻辑生成禁区外防守点。
     * 中间区域封在禁区矩形前沿；两侧区域封在圆弧外沿。
     */
    point2f official_defence_point(const point2f& threat, float penalty_buffer)
    {
        const point2f& goal = FieldPoint::Goal_Center_Point;
        const point2f& arc_right = FieldPoint::Penalty_Arc_Center_Right;
        const point2f& arc_left = FieldPoint::Penalty_Arc_Center_Left;
        const float dir = (threat - goal).angle();

        if (threat.y > arc_right.y || threat.y < arc_left.y)
        {
            const float radius = static_cast<float>(
                PENALTY_AREA_R + MAX_ROBOT_SIZE + penalty_buffer
            );
            return clamp_to_field(goal + Maths::vector2polar(radius, dir));
        }

        // 中间矩形的前沿为 x = -FIELD_LENGTH_H + PENALTY_AREA_R。
        const float front_x = static_cast<float>(
            -FIELD_LENGTH_H + PENALTY_AREA_R + MAX_ROBOT_SIZE + penalty_buffer
        );
        const float dx = front_x - goal.x;
        float y = goal.y;

        // 在主射门射线上求与禁区前沿的交点。
        if (fabs(threat.x - goal.x) > 0.001f)
        {
            y += (threat.y - goal.y) * dx / (threat.x - goal.x);
        }

        // 保持在矩形前沿附近，避免射线异常时跑到边线。
        const float middle_limit = static_cast<float>(PENALTY_AREA_L / 2 + 22.0);
        y = clampf(y, -middle_limit, middle_limit);
        return clamp_to_field(point2f(front_x, y));
    }

    int nearest_opponent_to_ball(const WorldModel* model)
    {
        int best_id = -1;
        float best_dist = 1000000.0f;
        const point2f& ball = model->get_ball_pos();
        const bool* exists = model->get_opp_exist_id();

        for (int id = 0; id < MAX_TEAM_ROBOTS; ++id)
        {
            if (!exists[id]) continue;
            const float dist = (model->get_opp_player_pos(id) - ball).length();
            if (dist < best_dist)
            {
                best_dist = dist;
                best_id = id;
            }
        }
        return best_id;
    }

    // 除持球人外，选择最靠近我方球门的对手作为第二威胁。
    int second_threat_opponent(const WorldModel* model, int ball_owner)
    {
        int best_id = -1;
        float best_score = 1000000.0f;
        const point2f& goal = FieldPoint::Goal_Center_Point;
        const point2f& ball = model->get_ball_pos();
        const bool* exists = model->get_opp_exist_id();
        const int opp_goalie = model->get_opp_goalie();

        for (int id = 0; id < MAX_TEAM_ROBOTS; ++id)
        {
            if (!exists[id] || id == ball_owner || id == opp_goalie) continue;

            const point2f& pos = model->get_opp_player_pos(id);
            // 越靠近我方门、越容易接到球，危险分越低。
            const float score = (pos - goal).length() + 0.35f * (pos - ball).length();
            if (score < best_score)
            {
                best_score = score;
                best_id = id;
            }
        }
        return best_id;
    }

    PlayerTask goalie_plan(const WorldModel* model, int robot_id,
        float depth, float y_limit)
    {
        PlayerTask task;
        const point2f& goal = FieldPoint::Goal_Center_Point;
        const point2f& ball = model->get_ball_pos();
        const point2f& goalie = model->get_our_player_pos(robot_id);
        const float shot_dir = (ball - goal).angle();

        point2f target = goal + Maths::vector2polar(depth, shot_dir);
        target.y = clampf(target.y, -y_limit, y_limit);
        target.x = clampf(target.x, goal.x + 4.0f, goal.x + depth);

        task.target_pos = target;
        task.orientate = (ball - goalie).angle();
        task.needCb = false;
        task.needKick = false;
        task.isPass = false;
        task.role = GoalieRole;
        return task;
    }

    void smooth_task(PlayerTask& task, int robot_id, float smooth)
    {
        static bool initialized[MAX_TEAM_ROBOTS] = { false };
        static point2f last_pos[MAX_TEAM_ROBOTS];
        static float last_dir[MAX_TEAM_ROBOTS] = { 0.0f };

        if (!valid_team_id(robot_id)) return;

        if (!initialized[robot_id])
        {
            initialized[robot_id] = true;
            last_pos[robot_id] = task.target_pos;
            last_dir[robot_id] = static_cast<float>(task.orientate);
            return;
        }

        last_pos[robot_id] = last_pos[robot_id] * smooth
            + task.target_pos * (1.0f - smooth);

        const float dir_error = Maths::normalizeAngle(
            static_cast<float>(task.orientate) - last_dir[robot_id]
        );
        last_dir[robot_id] = Maths::normalizeAngle(
            last_dir[robot_id] + dir_error * (1.0f - smooth)
        );

        task.target_pos = last_pos[robot_id];
        task.orientate = last_dir[robot_id];
    }
}

PlayerTask player_plan(const WorldModel* model, int robot_id)
{
    PlayerTask task;
    if (model == NULL || !valid_team_id(robot_id)) return task;

    const bool* our_exists = model->get_our_exist_id();
    if (our_exists == NULL || !our_exists[robot_id]) return task;

    const bool is_sim = model->get_simulation();
    const float penalty_buffer = is_sim
        ? SIM_DEF3_PENALTY_BUFFER : REAL_DEF3_PENALTY_BUFFER;
    const float cover_angle = is_sim
        ? SIM_DEF3_COVER_ANGLE : REAL_DEF3_COVER_ANGLE;
    const float teammate_gap = is_sim
        ? SIM_DEF3_TEAMMATE_GAP : REAL_DEF3_TEAMMATE_GAP;
    const float smooth = is_sim ? SIM_DEF3_SMOOTH : REAL_DEF3_SMOOTH;
    const float goalie_depth = is_sim
        ? SIM_DEF3_GOALIE_DEPTH : REAL_DEF3_GOALIE_DEPTH;
    const float goalie_y_limit = is_sim
        ? SIM_DEF3_GOALIE_Y_LIMIT : REAL_DEF3_GOALIE_Y_LIMIT;

    const int goalie_id = model->get_our_goalie();
    if (robot_id == goalie_id)
    {
        task = goalie_plan(model, robot_id, goalie_depth, goalie_y_limit);
        smooth_task(task, robot_id, smooth);
        return task;
    }

    // 3V3 中守门员之外最多取两台车，按车号稳定分配主后卫/协防后卫。
    int defenders[2] = { -1, -1 };
    int defender_count = 0;
    for (int id = 0; id < MAX_TEAM_ROBOTS && defender_count < 2; ++id)
    {
        if (our_exists[id] && id != goalie_id)
        {
            defenders[defender_count++] = id;
        }
    }

    int role_index = -1;
    if (robot_id == defenders[0]) role_index = 0;
    if (robot_id == defenders[1]) role_index = 1;

    // 若配置中意外出现超过三台车，多出来的车原地面向球，避免破坏防线。
    if (role_index < 0)
    {
        const point2f& self = model->get_our_player_pos(robot_id);
        task.target_pos = self;
        task.orientate = (model->get_ball_pos() - self).angle();
        return task;
    }

    const point2f& ball = model->get_ball_pos();
    const point2f& goal = FieldPoint::Goal_Center_Point;
    const point2f& self = model->get_our_player_pos(robot_id);

    const point2f primary_target = official_defence_point(ball, penalty_buffer);
    point2f secondary_target;

    const int ball_owner = nearest_opponent_to_ball(model);
    const int second_threat = second_threat_opponent(model, ball_owner);
    if (second_threat >= 0)
    {
        secondary_target = official_defence_point(
            model->get_opp_player_pos(second_threat), penalty_buffer
        );
    }
    else
    {
        // 没有可识别的第二威胁时，在主射门线的另一侧补位。
        float ball_dir = (ball - goal).angle();
        const float side = ball.y >= 0.0f ? -1.0f : 1.0f;
        const point2f cover_threat = goal + Maths::vector2polar(
            300.0f, ball_dir + side * cover_angle
        );
        secondary_target = official_defence_point(cover_threat, penalty_buffer);
    }

    // 防止两名后卫目标点重叠：协防后卫沿防线切向让开。
    point2f separation = secondary_target - primary_target;
    if (separation.length() < teammate_gap)
    {
        const float main_dir = (primary_target - goal).angle();
        const float side = ball.y >= 0.0f ? -1.0f : 1.0f;
        const point2f cover_threat = goal + Maths::vector2polar(
            300.0f, main_dir + side * cover_angle
        );
        secondary_target = official_defence_point(cover_threat, penalty_buffer);
    }

    task.target_pos = role_index == 0 ? primary_target : secondary_target;
    task.orientate = (ball - self).angle();
    task.needCb = false;
    task.needKick = false;
    task.isPass = false;
    task.role = role_index == 0 ? LeftBack : RightBack;

    smooth_task(task, robot_id, smooth);
    return task;
}

#endif
