/*
Author(s): Djordje Vukcevic, Sven Schneider
Institute: Hochschule Bonn-Rhein-Sieg

Copyright (c) [2018]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <safety_controller.hpp>

safety_controller::safety_controller(youbot_mediator &robot_driver,
                                     const bool print_logs):
    robot_driver_(robot_driver),
    robot_chain_(robot_driver_.get_robot_model()),
    NUMBER_OF_JOINTS_(robot_chain_.getNrOfJoints()),
    NUMBER_OF_SEGMENTS_(robot_chain_.getNrOfSegments()),
    NUMBER_OF_FRAMES_(robot_chain_.getNrOfSegments() + 1),
    NUMBER_OF_CONSTRAINTS_(6),
    predictor_(robot_chain_),
    joint_position_limits_max_(robot_driver_.get_maximum_joint_pos_limits()),
    joint_position_limits_min_(robot_driver_.get_minimum_joint_pos_limits()),
    joint_position_thresholds_(robot_driver_.get_joint_position_thresholds()),
    joint_velocity_limits_(robot_driver_.get_joint_velocity_limits()),
    joint_torque_limits_(robot_driver_.get_joint_torque_limits()),
    temp_state_(NUMBER_OF_JOINTS_, NUMBER_OF_SEGMENTS_, 
                NUMBER_OF_FRAMES_, NUMBER_OF_CONSTRAINTS_),
    predicted_states_(3, temp_state_),            
    PRINT_LOGS_(print_logs)
{

}
        
int safety_controller::generate_commands(const state_specification &current_state,
                                         state_specification &commands,
                                         const double dt_sec,
                                         const int desired_control_mode,
                                         const int prediction_method)
{
    assert(NUMBER_OF_JOINTS_ == commands.qd.rows()); 
    assert(NUMBER_OF_JOINTS_ == current_state.qd.rows());
    /* 
        First Safety Level: Is the Current State Safe?
        I.e. Check for NaN and infinite values in the commands variables.
        Stop the motion if some of the values are NaN or infinite.
        And check if robot has already crossed the position and velocity limits.
        Also check if robot is close to position limits, 
        given the measured (not integrated) angles and velocities.
        If everything ok, proceed to the second level.
    */
    if (!is_current_state_safe(current_state)) 
        return control_mode::STOP_MOTION;

    /*
        Make predictions - Integrate joint accelerations to velocities and positions
        I.e. generate initial commands and predict where the robot will end-up
        in next two steps, if the computed commands have been applied.
    */
    predictor_.integrate_joint_space(current_state, predicted_states_, dt_sec, 
                                     2, prediction_method, false, false);

    // Write computed torques, predicted velocities & positions in command state
    set_commands(commands, current_state);

    /*
        Second Safety Level: Is the Future State Safe?
        I.e. Check if the commads from desired control interface will 
        make robot go over the limits.
        If yes: switch to another mode, and/or scale down the commands,
        or stop the robot.
        If not: continue with the original commands.
    */
    return check_future_state(commands, desired_control_mode);
}

bool safety_controller::is_current_state_safe(
                                    const state_specification &current_state)
{
    /* 
        Check current velocities and position for every case. 
        Our model is not correct and commanded torque in previous iteration,
        may produce too high velocities or move joint over position limits.
        Basically check for error in model evaluation and predictions.
        TODO: Investigate if its necessary also to check for mesured torques.
        It depends on:
            - which torque we command, full or constraint
            - what sensor readings we receive: full torqe on the joint, 
                                        or just torque produced by motors
            - question is: what torques do you command if e.g. RNE is used?
                        do you have a choice?
                        e.g. gravity compenzation
    */
    for (int i = 0; i < NUMBER_OF_JOINTS_; i++)
    {
        if(!is_state_finite(current_state, i) || \
           velocity_limit_reached(current_state, i) || \
           position_limit_reached(current_state, i) || \
           reaching_position_limits(current_state, i))
           {
                if (PRINT_LOGS_) 
                    std::cout << "Current robot state is not safe" << std::endl;
               return false;
           } 
    }
    return true;
}

void safety_controller::set_commands(state_specification &commands,
                                     const state_specification &current_state)
{
    commands.control_torque = current_state.control_torque;
    commands.qdd = current_state.qdd;
    commands.qd = predicted_states_[0].qd;
    commands.q = predicted_states_[0].q;
}

int safety_controller::check_future_state(const state_specification &commands,
                                          const int desired_control_mode)
{
    switch (desired_control_mode)
    {   
        case control_mode::TORQUE:
            return check_torques(commands);

        case control_mode::VELOCITY:
            return check_velocities(commands);

        case control_mode::POSITION:
            return check_positions(commands);
    
        default: return control_mode::STOP_MOTION;
    }
}

bool safety_controller::is_state_finite(const state_specification &current_state, 
                                        const int joint)
{
    if (!std::isfinite(current_state.control_torque(joint))){
        if (PRINT_LOGS_) 
            std::cout << "Computed torque for joint: "<< joint + 1 
                      <<" is not finite!" << std::endl;
        return false;
    }
    else if (!std::isfinite(current_state.qdd(joint))){
        if (PRINT_LOGS_) 
            std::cout << "Computed joint "<< joint + 1 
                      <<" acceleration is not finite!" << std::endl;
        return false;
    }
    else if (!std::isfinite(current_state.qd(joint))){
        if (PRINT_LOGS_) 
            std::cout << "Computed joint "<< joint + 1 
                      <<" velocity is not finite!" << std::endl;
        return false;
    }
    else if (!std::isfinite(current_state.q(joint))){
        if (PRINT_LOGS_) 
            std::cout << "Computed joint "<< joint + 1
                      <<" position is not finite!" << std::endl;
        return false;
    }
    else return true;
}

bool safety_controller::torque_limit_reached(const state_specification &state,
                                             const int joint)
{
    if (fabs(state.control_torque(joint)) >= joint_torque_limits_[joint])
    {
        if(PRINT_LOGS_) 
            std::cout << "Joint " << joint + 1 
                      << " torque limit reached!" << std::endl;
        return true;        
    } else return false;
}

bool safety_controller::velocity_limit_reached(const state_specification &state,
                                               const int joint)
{
    if (fabs(state.qd(joint)) >= joint_velocity_limits_[joint])
    {
        if(PRINT_LOGS_) 
            std::cout << "Joint " << joint + 1 
                      << " velocity limit reached!" << std::endl;
        return true;        
    } else return false;
}

bool safety_controller::position_limit_reached(const state_specification &state,
                                               const int joint)
{
    if ((state.q(joint) >= joint_position_limits_max_[joint]) || \
        (state.q(joint) <= joint_position_limits_min_[joint]))
    {
        if(PRINT_LOGS_) 
            std::cout << "Joint " << joint + 1 
                      << " position limit reached!" << std::endl;
        return true; 
    } else return false;
}

bool safety_controller::reaching_position_limits(const state_specification &state,
                                                 const int joint)
{

    if ((joint_position_limits_max_[joint] - state.q(joint)) \
        < joint_position_thresholds_[joint])
    {
        if(state.qd(joint) > 0.02)
        {
            std::cout << "Joint " << joint + 1 << " is too close to the max limit"
                      << std::endl;
            return true;
        } 
    } 
    
    else if ((joint_position_limits_min_[joint] - state.q(joint)) \
             > -joint_position_thresholds_[joint])
    {
        if(state.qd(joint) < -0.02)
        {
            std::cout << "Joint " << joint + 1 << " is too close to the min limit"
                      << std::endl;
            return true;
        }
    } 
    else return false;
}

/*
    Check if the commaned torques are over torque limits.
    Check if the commanded torqes will make a joint go over the position limits.
    If all ok: continue with this control mode.
    Else: stop the robot.
*/
int safety_controller::check_torques(const state_specification &commands)
{

    for (int i = 0; i < NUMBER_OF_JOINTS_; i++)
    {
        if(torque_limit_reached(commands, i) || \
           position_limit_reached(predicted_states_[0], i) || \
           position_limit_reached(predicted_states_[1], i))
        {
            if (PRINT_LOGS_) 
                std::cout << "Torque commands are not safe" << std::endl;
            return control_mode::STOP_MOTION;
        }
    } return control_mode::TORQUE;
}

int safety_controller::check_velocities(const state_specification &commands)
{
    //If a velocity limit reached, rescale velocities or stop the control
    for (int i = 0; i < NUMBER_OF_JOINTS_; i++)
    {
        if (velocity_limit_reached(commands, i) || \
            position_limit_reached(predicted_states_[0], i) || \
            position_limit_reached(predicted_states_[1], i))
        {
            if (PRINT_LOGS_) 
                std::cout << "Velocity commands are not safe" << std::endl;
            return control_mode::STOP_MOTION;
        }
    }
    return control_mode::VELOCITY;
}

/*
    Simple check if the computed postion commands are valid, i.e. over the limit.
    Last step in safety check.
*/
int safety_controller::check_positions(const state_specification &commands)
{
    //If a position limit reached, stop the program
    for (int i = 0; i < NUMBER_OF_JOINTS_; i++)
    {
        if (position_limit_reached(commands, i))
        {
            if (PRINT_LOGS_)
                std::cout << "Position commands are not safe" << std::endl;
            return control_mode::STOP_MOTION;
        }
    }
    return control_mode::POSITION;
}

bool safety_controller::reduce_velocities(const state_specification &commands)
{
    //TODO: Maybe not necessary
    return true;
}