/*
Author(s): Djordje Vukcevic, Sven Schneider
Institute: Hochschule Bonn-Rhein-Sieg
Description: Mediator component for enabling conversion of data types.
Acknowledgment: This sofware component is based on Jeyaprakash Rajagopal's 
master thesis code.

Copyright (c) [2019]

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

#ifndef YOUBOT_CONSTANTS_HPP
#define YOUBOT_CONSTANTS_HPP

#include <stdlib.h> /* abs */
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>

namespace youbot_constants
{
    // Number of joints in the manipulator
    const int NUMBER_OF_JOINTS_(5);
    const int NUMBER_OF_SEGMENTS_(5);
    const int NUMBER_OF_FRAMES_(6);

    //Arm's root acceleration
    const std::vector<double> root_acceleration_ {0.0, 0.0, 9.81, 0.0, 0.0, 0.0};

    //Kuka youBot store position limit values: positive and negative
    const std::vector<double> joint_position_limits_max_1_ {  2.9496,  1.5707,  2.5481,  1.7889,  2.9234};
    const std::vector<double> joint_position_limits_min_1_ { -2.9496, -1.1344, -2.6354, -1.7889, -2.9234};

    //Position limit values from URDF file: positive and negative
    const std::vector<double> joint_position_limits_max_2_ {5.89921, 2.70526,  0.00000, 3.57792, 5.84685};
    const std::vector<double> joint_position_limits_min_2_ {0.00000, 0.00000, -5.16617, 0.00000, 0.00000};
    
    const std::vector<double> joint_position_soft_threshold {0.00000, 0.00000, -5.16617, 0.00000, 0.00000};

    // Robocup URDF file parameters for velocity limits
    // std::vector<double> joint_velocity_limits_ {1.5707, 0.8, 1.0, 1.5707, 1.5707};

    // YouBot Store velocity limits
    const std::vector<double> joint_velocity_limits_ {1.5707, 01.5707, 1.5707, 1.5707, 1.5707};

    // JP's max torques
    // std::vector<double> joint_torque_limits_ {12.9012, 12.9012, 8.2700, 4.1748, 1.7550};

    // youBot store's max torques 
    const std::vector<double> joint_torque_limits_ {9.5, 9.5, 6.0, 2.0, 1.0};

    // Benjamin Keiser' max torques (fast version)
    // std::vector<double> joint_torque_limits_ {17.0, 17.0, 8.0, 2.7, 1.0}};

    // Offsets required for the youBot store model: Negative Candle config values -Robocup
    // std::vector<double> youbot_joint_offsets_ {-2.1642, -1.13446, 2.54818, -1.78896, -2.9234};

    // Offsets required for the youBot store model: Negative Candle config values - JP 
    const std::vector<double> joint_offsets_ {-2.94960, -1.13446, 2.54818, -1.78896, -2.9234};

    // Offsets required for the youBot store model: Negative Candle config values -Sven's
    // std::vector<double> joint_offsets_ {-2.9496, -1.1344, 2.6354, -1.7890, -2.9234};

    // Offsets required for the youBot store model: Negative Candle config values: keiser's
    // std::vector<double> joint_offsets_ {-2.9496, -1.1344, 2.5481, -1.7889, -2.9234};

    // Rotor inertia - "d" in the algorithm: Computed from youBot store values
    const std::vector<double> joint_inertia_ {0.33848, 0.33848, 0.13571, 0.04698, 0.01799};
}

#endif /* YOUBOT_CONSTANTS_HPP */