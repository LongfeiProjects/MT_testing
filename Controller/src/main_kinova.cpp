/*
Author(s): Djordje Vukcevic, Sven Schneider
Institute: Hochschule Bonn-Rhein-Sieg
Description: Mediator component for enabling conversion of data types.

Copyright (c) [2020]

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
#include <kinova_mediator.hpp>
#include <state_specification.hpp>
#include <dynamics_controller.hpp>
#include <solver_recursive_newton_euler.hpp>
#include <fk_vereshchagin.hpp>
#include <geometry_utils.hpp>
#include <model_prediction.hpp>
#include <safety_controller.hpp>
#include <finite_state_machine.hpp>
#include <motion_profile.hpp>

#define IP_ADDRESS_1 "192.168.1.10"
#define IP_ADDRESS_2 "192.168.1.12"
#define PORT 10000

enum desired_pose
{
    CANDLE       = 0,
    HOME         = 1,
    RETRACT      = 2,
    PACKAGING    = 3
};

enum path_types
{
    SINE_PATH = 0,
    STEP_PATH = 1,
    INF_SIGN_PATH = 2
};

// Waiting time during actions
constexpr auto TIMEOUT_DURATION      = std::chrono::seconds{20};
std::chrono::steady_clock::time_point loop_start_time;
std::chrono::duration <double, std::micro> loop_interval{};

const int SECOND                     = 1000000;
const int MILLISECOND                = 1000;
const int JOINTS                     = 7;
const int NUMBER_OF_CONSTRAINTS      = 6;
const int desired_dynamics_interface = dynamics_interface::CART_ACCELERATION;
const int abag_error_type            = error_type::SIGN;
int RATE_HZ                          = 1000; // Hz
int motion_profile_id                = m_profile::CONSTANT;
int path_type                        = path_types::STEP_PATH;
int desired_pose_id                  = desired_pose::HOME;
int desired_task_model               = task_model::full_pose;
int desired_control_mode             = control_mode::TORQUE;
int environment                      = kinova_environment::SIMULATION;
int robot_model_id                   = kinova_model::URDF;
int id                               = robot_id::KINOVA_GEN3_1;
int control_loop_delay_count         = 0;
const double time_horizon_amplitude  = 2.5;
double tube_speed                    = 0.01;
double desired_null_space_angle      = 90.0; // Unit degrees
double task_time_limit_sec           = 600.0;

bool log_data                        = false;
bool control_null_space              = false;
bool compensate_gravity              = false;
bool use_mass_alternation            = false;

std::vector<bool> control_dims       = {true, true, true, // Linear
                                        false, false, false}; // Angular

// Last parameter: Numer of points
const std::vector<double> path_parameters = {0.5, 3.5, 0.05, 0.008, 12};
std::vector<double> tube_start_position   = {0.0, 0.0, 0.0};
std::vector<double> tube_tolerances       = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

std::vector< std::vector<double> > tube_path_points(path_parameters[4], std::vector<double>(3, 0.0));
std::vector< std::vector<double> > path_poses(path_parameters[4] - 1,   std::vector<double>(12, 0.0));

const Eigen::VectorXd max_command         = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) << 20.0, 20.0, 20.0, 10.0, 10.0, 10.0).finished();

// Full Pose ABAG parameters
const Eigen::VectorXd error_alpha         = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.900000, 0.900000, 0.900000, 
                                               0.850000, 0.850000, 0.850000).finished();
const Eigen::VectorXd bias_threshold      = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.000407, 0.000407, 0.000407, 
                                               0.001007, 0.001007, 0.001007).finished();
const Eigen::VectorXd bias_step           = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.000495, 0.000495, 0.000495, 
                                               0.003495, 0.003495, 0.003495).finished();
const Eigen::VectorXd gain_threshold      = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.552492, 0.552492, 0.552492, 
                                               0.252492, 0.252492, 0.252492).finished();
const Eigen::VectorXd gain_step           = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.003152, 0.003152, 0.003152, 
                                               0.015152, 0.015152, 0.015152).finished();

// moveGuarded-torque ABAG parameters
const Eigen::VectorXd error_alpha_1         = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.850000, 0.900000, 0.900000, 
                                               0.850000, 0.850000, 0.850000).finished();
const Eigen::VectorXd bias_threshold_1      = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.000407, 0.000407, 0.000407, 
                                               0.001007, 0.001007, 0.001007).finished();
const Eigen::VectorXd bias_step_1           = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.000550, 0.000495, 0.000495, 
                                               0.003495, 0.003495, 0.003495).finished();
const Eigen::VectorXd gain_threshold_1      = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.552492, 0.552492, 0.552492, 
                                               0.252492, 0.252492, 0.252492).finished();
const Eigen::VectorXd gain_step_1           = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.003152, 0.003152, 0.003152, 
                                               0.015152, 0.015152, 0.015152).finished();

// moveTo-torque ABAG parameters
const Eigen::VectorXd error_alpha_2         = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.900000, 0.900000, 0.900000, 
                                               0.850000, 0.850000, 0.850000).finished();
const Eigen::VectorXd bias_threshold_2      = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.000457, 0.000407, 0.000407, 
                                               0.001007, 0.001007, 0.001007).finished();
const Eigen::VectorXd bias_step_2           = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.000500, 0.000400, 0.000400, 
                                               0.003495, 0.003495, 0.003495).finished();
const Eigen::VectorXd gain_threshold_2      = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.502492, 0.502492, 0.502492, 
                                               0.252492, 0.252492, 0.252492).finished();
const Eigen::VectorXd gain_step_2           = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.002552, 0.002552, 0.002552, 
                                               0.015152, 0.015152, 0.015152).finished();

// moveTo-follow_path-torque ABAG parameters
const Eigen::VectorXd error_alpha_3         = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.850000, 0.850000, 0.850000, 
                                               0.850000, 0.850000, 0.850000).finished();
const Eigen::VectorXd bias_threshold_3      = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.000457, 0.000407, 0.000407, 
                                               0.001007, 0.001007, 0.001007).finished();
const Eigen::VectorXd bias_step_3           = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.000550, 0.000550, 0.000550, 
                                               0.003495, 0.003495, 0.003495).finished();
const Eigen::VectorXd gain_threshold_3      = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.502492, 0.454092, 0.454092, 
                                               0.252492, 0.252492, 0.252492).finished();
const Eigen::VectorXd gain_step_3           = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.003152, 0.003552, 0.003552, 
                                               0.015152, 0.015152, 0.015152).finished();

// moveTo_weight_compensation-torque ABAG parameters
const Eigen::VectorXd error_alpha_4         = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.900000, 0.900000, 0.900000, 
                                               0.850000, 0.850000, 0.850000).finished();
const Eigen::VectorXd bias_threshold_4      = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.000550, 0.000457, 0.000457, 
                                               0.001007, 0.001007, 0.001007).finished();
const Eigen::VectorXd bias_step_4           = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.000500, 0.000450, 0.000450, 
                                               0.003495, 0.003495, 0.003495).finished();
const Eigen::VectorXd gain_threshold_4      = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.452492, 0.452492, 0.452492, 
                                               0.252492, 0.252492, 0.252492).finished();
const Eigen::VectorXd gain_step_4           = (Eigen::VectorXd(NUMBER_OF_CONSTRAINTS) \
                                            << 0.002600, 0.002600, 0.002600, 
                                               0.015152, 0.015152, 0.015152).finished();

// Stop Motion control parameters used by the ABAG -> parameters specific each robot type... test these values
const Eigen::VectorXd STOP_MOTION_ERROR_ALPHA    = (Eigen::VectorXd(JOINTS) << 0.800000, 0.800000, 0.800000, 0.800000, 0.800000, 0.800000, 0.800000).finished();
const Eigen::VectorXd STOP_MOTION_BIAS_THRESHOLD = (Eigen::VectorXd(JOINTS) << 0.000557, 0.006000, 0.000557, 0.006500, 0.000457, 0.006500, 0.000457).finished();
const Eigen::VectorXd STOP_MOTION_BIAS_STEP      = (Eigen::VectorXd(JOINTS) << 0.000900, 0.002500, 0.000900, 0.002000, 0.000500, 0.002000, 0.000500).finished();
const Eigen::VectorXd STOP_MOTION_GAIN_THRESHOLD = (Eigen::VectorXd(JOINTS) << 0.602492, 0.500000, 0.602492, 0.500000, 0.602492, 0.500000, 0.602492).finished();
const Eigen::VectorXd STOP_MOTION_GAIN_STEP      = (Eigen::VectorXd(JOINTS) << 0.005552, 0.010552, 0.005552, 0.010552, 0.003552, 0.010552, 0.003552).finished();

const Eigen::VectorXd min_bias_sat               = Eigen::VectorXd::Constant(6, -1.0);
const Eigen::VectorXd min_command_sat            = Eigen::VectorXd::Constant(6, -1.0);
const Eigen::VectorXd null_space_abag_parameters = (Eigen::VectorXd(6) << 0.1, 0.1, 0.1, 0.1, 0.1, 0.1).finished(); // Last param is max command

//  Parameters for weight compensation: x_bias-offset (-1.0 <-> 1.0), y_bias-offset, z_bias-offset, K proportional, error-tube(0.0 <-> 1.0),
//                                      bias-variance, gain-variance, bias slope, 
//                                      control-period, x_max_trigger_count, y_max_trigger_count, z_max_trigger_count
const Eigen::VectorXd compensation_parameters = (Eigen::VectorXd(12) << -0.08, -0.07, 0.0, 1.2, 0.015,
                                                                         0.00016, 0.0025, 0.00002,
                                                                         60, 6, 3, 3).finished();

// Create an event listener that will set the promise action event to the exit value
// Will set promise to either END or ABORT
// Use finish_promise.get_future.get() to wait and get the value
std::function<void(Kinova::Api::Base::ActionNotification)> create_event_listener_by_promise(std::promise<Kinova::Api::Base::ActionEvent>& finish_promise)
{
    return [&finish_promise] (Kinova::Api::Base::ActionNotification notification)
    {
        const auto action_event = notification.action_event();
        switch(action_event)
        {
            case Kinova::Api::Base::ActionEvent::ACTION_END:
            case Kinova::Api::Base::ActionEvent::ACTION_ABORT:
                finish_promise.set_value(action_event);
                break;
            default:
                break;
        }
    };
}

/*****************************
 * Example related function *
 *****************************/
int64_t GetTickUs()
{
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    return (start.tv_sec * 1000000LLU) + (start.tv_nsec / 1000);
}

//Make sure that the control loop runs exactly with the specified frequency
int enforce_loop_frequency(const int dt)
{
    loop_interval = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - loop_start_time);

    if (loop_interval < std::chrono::microseconds(dt)) // Loop is sufficiently fast
    {
        while (loop_interval < std::chrono::microseconds(dt - 1))
            loop_interval = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - loop_start_time);

        return 0;
    }
    else return -1; //Loop is too slow
}

// Define the callback function used in Refresh_callback
auto lambda_fct_callback = [](const Kinova::Api::Error &err, const Kinova::Api::BaseCyclic::Feedback data)
{
    // We are printing the data of the moving actuator just for the example purpose,
    // avoid this in a real-time loop
    std::string serialized_data;
    google::protobuf::util::MessageToJsonString(data.actuators(6), &serialized_data);
    std::cout << serialized_data << std::endl << std::endl;
};


void rotate_joint(kinova_mediator &robot_driver, const int joint, const double rate)
{
    robot_driver.set_control_mode(control_mode::VELOCITY);
    KDL::JntArray rotate_joint(JOINTS);
    rotate_joint(joint) = rate; 
    robot_driver.set_joint_velocities(rotate_joint);
    if (environment != kinova_environment::SIMULATION) usleep(5000 * MILLISECOND);
}

int go_to(kinova_mediator &robot_driver, const int desired_pose_)
{
    std::vector<double> configuration_array(7, 0.0);
    // Angle value are in units of degree
    switch (desired_pose_)
    {
        case desired_pose::CANDLE:
            configuration_array = std::vector<double> {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            break;
        case desired_pose::PACKAGING:
            configuration_array = std::vector<double> {0.0, 330.0, 180.0, 214.0, 0.0, 115.0, 270.0};
            break;
        case desired_pose::RETRACT:
            configuration_array = std::vector<double> {0.0, 340.0, 180.0, 214.0, 0.0, 310.0, 90.0};
            break;       
        default:
            configuration_array = std::vector<double> {0.0, 15.0, 180.0, 230.0, 0.0, 55.0, 90.0};
            break;
    }

    if (environment != kinova_environment::SIMULATION)
    {
        // Extract user password from a file
        fstream newfile;
        string kinova_passwd;
        newfile.open("/home/djole/Master/Thesis/GIT/MT_testing/kinova_passwd.txt", ios::in);
        if (newfile.is_open())
        {
            if (id == KINOVA_GEN3_1) getline(newfile, kinova_passwd);
            else while (getline(newfile, kinova_passwd)) {}
            newfile.close();
        }


        // Create API objects
        auto error_callback = [](Kinova::Api::KError err){ cout << "_________ callback error _________" << err.toString(); };
        
        auto transport = new Kinova::Api::TransportClientTcp();
        auto router = new Kinova::Api::RouterClient(transport, error_callback);
        if (id == KINOVA_GEN3_1) transport->connect(IP_ADDRESS_1, PORT);
        else transport->connect(IP_ADDRESS_2, PORT);

        // Set session data connection information
        auto create_session_info = Kinova::Api::Session::CreateSessionInfo();
        create_session_info.set_username("admin");
        create_session_info.set_password(kinova_passwd);
        create_session_info.set_session_inactivity_timeout(200);   // (milliseconds)
        create_session_info.set_connection_inactivity_timeout(200); // (milliseconds)

        // Session manager service wrapper
        // std::cout << "Creating sessions for communication" << std::endl;
        auto session_manager = new Kinova::Api::SessionManager(router);
        session_manager->CreateSession(create_session_info);
        // std::cout << "Sessions created" << std::endl;

        // Create services
        auto base = new Kinova::Api::Base::BaseClient(router);
        // auto actuator_config = new Kinova::Api::ActuatorConfig::ActuatorConfigClient(router);

        // Make sure the arm is in Single Level Servoing before executing an Action
        auto servoingMode = Kinova::Api::Base::ServoingModeInformation();
        servoingMode.set_servoing_mode(Kinova::Api::Base::ServoingMode::SINGLE_LEVEL_SERVOING);
        base->SetServoingMode(servoingMode);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        auto constrained_joint_angles = Kinova::Api::Base::ConstrainedJointAngles();
        auto joint_angles = constrained_joint_angles.mutable_joint_angles();
        auto actuator_count = base->GetActuatorCount();

        for (size_t i = 0; i < actuator_count.count(); ++i) 
        {
            auto joint_angle = joint_angles->add_joint_angles();
            joint_angle->set_joint_identifier(i);
            joint_angle->set_value(configuration_array[i]);
        }

        // Connect to notification action topic (Promise alternative)
        // See cartesian examples for Reference alternative
        std::promise<Kinova::Api::Base::ActionEvent> finish_promise;
        auto finish_future = finish_promise.get_future();
        auto promise_notification_handle = base->OnNotificationActionTopic(
            create_event_listener_by_promise(finish_promise),
            Kinova::Api::Common::NotificationOptions()
        );

        // std::cout << "Reaching joint angles..." << std::endl;
        base->PlayJointTrajectory(constrained_joint_angles);

        // Wait for future value from promise (Promise alternative)
        // See cartesian examples for Reference alternative
        const auto status = finish_future.wait_for(TIMEOUT_DURATION);
        base->Unsubscribe(promise_notification_handle);

        if (status != std::future_status::ready)
        {
            std::cout << "Timeout on action notification wait" << std::endl;
            std::cout << "Can't reach safe position, exiting" << std::endl;

            // Close API session
            session_manager->CloseSession();

            // Deactivate the router and cleanly disconnect from the transport object
            router->SetActivationStatus(false);
            transport->disconnect();

            // Destroy the API
            delete base;
            delete session_manager;
            delete router;
            delete transport;
            return -1;
        }

        // const auto promise_event = finish_future.get();

        // std::cout << "Joint angles reached" << std::endl;
        // std::cout << "Promise value : " << Kinova::Api::Base::ActionEvent_Name(promise_event) << std::endl;

        /**
         * Code for calibrating torque offets. 
         * Should be used only once and the robot must be in the zero-configuration.
         */
        // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        // auto torque_offset_message = Kinova::Api::ActuatorConfig::TorqueOffset();
        // torque_offset_message.set_torque_offset(0.0f);
        // actuator_config->SetTorqueOffset(torque_offset_message, 1);

        // torque_offset_message = Kinova::Api::ActuatorConfig::TorqueOffset();
        // torque_offset_message.set_torque_offset(0.00253617f);
        // // torque_offset_message.set_torque_offset(0.0f);
        // actuator_config->SetTorqueOffset(torque_offset_message, 2);

        // torque_offset_message = Kinova::Api::ActuatorConfig::TorqueOffset();
        // torque_offset_message.set_torque_offset(0.0f);
        // actuator_config->SetTorqueOffset(torque_offset_message, 3);

        // torque_offset_message = Kinova::Api::ActuatorConfig::TorqueOffset();
        // torque_offset_message.set_torque_offset(0.00153136f);
        // // torque_offset_message.set_torque_offset(0.0f);
        // actuator_config->SetTorqueOffset(torque_offset_message, 4);

        // torque_offset_message = Kinova::Api::ActuatorConfig::TorqueOffset();
        // torque_offset_message.set_torque_offset(0.0f);
        // actuator_config->SetTorqueOffset(torque_offset_message, 5);

        // torque_offset_message = Kinova::Api::ActuatorConfig::TorqueOffset();
        // torque_offset_message.set_torque_offset(0.00137371f);
        // // torque_offset_message.set_torque_offset(0.0f);
        // actuator_config->SetTorqueOffset(torque_offset_message, 6);

        // torque_offset_message = Kinova::Api::ActuatorConfig::TorqueOffset();
        // torque_offset_message.set_torque_offset(0.0f);
        // actuator_config->SetTorqueOffset(torque_offset_message, 7);
        // std::this_thread::sleep_for(std::chrono::milliseconds(500));
        /**
         *  End of the calibration code
         */

        // auto control_mode = actuator_config->GetControlMode(7);
        // std::cout << "ControlMode: " << control_mode.control_mode() << std::endl;
        
        // auto torque_offset = actuator_config->GetTorqueOffset(7);
        // std::cout << "TorqueOffset: " << torque_offset.torque_offset() << std::endl;

        // Close API session
        session_manager->CloseSession();

        // Deactivate the router and cleanly disconnect from the transport object
        router->SetActivationStatus(false);
        transport->disconnect();

        // Destroy the API
        // delete actuator_config;
        delete base;
        delete session_manager;
        delete router;
        delete transport;
        // printf("High-Level Control Completed\n");
    }

    else
    {
        robot_driver.initialize(robot_model_id, environment, id);
        if (!robot_driver.is_initialized())
        {
            printf("Robot is not initialized\n");
            return -1;
        }

        if (robot_driver.set_control_mode(control_mode::POSITION) == -1)
        {
            printf("Incorrect control mode\n");
            return -1;
        }

        KDL::JntArray config(JOINTS);
        for (int i = 0; i < JOINTS; i++) 
            config(i) = DEG_TO_RAD(configuration_array[i]);  
        robot_driver.set_joint_positions(config);
    }
    return 0;
}

int define_task(dynamics_controller *dyn_controller)
{
    std::vector<double> desired_ee_pose(12, 0.0);

    // Create End_effector Cartesian Acceleration task
    dyn_controller->define_ee_acc_constraint(std::vector<bool>{false, false, false, // Linear
                                                               false, false, false}, // Angular
                                             std::vector<double>{0.0, 0.0, 0.0, // Linear
                                                                 0.0, 0.0, 0.0}); // Angular
    // Create External Forces task
    dyn_controller->define_ee_external_force(std::vector<double>{0.0, 0.0, 0.0, // Linear
                                                                 0.0, 0.0, 0.0}); // Angular
    // Create Feedforward torques task
    dyn_controller->define_feedforward_torque(std::vector<double>{0.0, 0.0, 0.0, 0.0,
                                                                  0.0, 0.0, 0.0});

    switch (desired_pose_id)
    {
        case desired_pose::CANDLE:
            tube_start_position = std::vector<double>{0.0, -0.0248591, 0.92586};
            desired_ee_pose     = { 0.0, -0.0248591, 1.12586, // Linear: Vector
                                    1.0, 0.0, 0.0, // Angular: Rotation matrix
                                    0.0, 1.0, 0.0,
                                    0.0, 0.0, 1.0};
            break;

        case desired_pose::RETRACT:
            tube_start_position = std::vector<double>{0.117804, 0.00134572, 0.489747};
            desired_ee_pose     = { 0.117804, 0.00134572, 0.389747, // Linear: Vector
                                    0.0, -0.997564, -0.0697565, // Angular: Rotation matrix
                                    1.0,  0.0,       0.0,
                                    0.0, -0.0697565, 0.997564};
            break;

        default:
            // HOME pose
            tube_start_position = std::vector<double>{0.39514, 0.00134662, 0.433724};
            desired_ee_pose     = { 0.65514, 0.01434662, 0.433724, // Linear: Vector
                                    0.0, 0.0, -1.0, // Angular: Rotation matrix
                                    1.0, 0.0, 0.0,
                                    0.0, -1.0, 0.0};
            break;
    }

    switch (desired_task_model)
    {
        case task_model::moveTo_follow_path:
            switch (path_type)
            {
                case path_types::STEP_PATH:
                    motion_profile::draw_step_xy(tube_path_points, 6, 0.001,
                                                 desired_ee_pose[0], desired_ee_pose[1], desired_ee_pose[2]);
                    break;
                
                case path_types::INF_SIGN_PATH:
                    motion_profile::draw_inf_sign_xy(tube_path_points, 0.5, 0.4, 0.18, 0.5, 
                                                     desired_ee_pose[0], desired_ee_pose[1], desired_ee_pose[2]);
                    break;

                case path_types::SINE_PATH:
                    motion_profile::draw_sine_xy(tube_path_points, path_parameters[0], path_parameters[1],
                                                 path_parameters[2], path_parameters[3], 
                                                 desired_ee_pose[0], desired_ee_pose[1], desired_ee_pose[2]);
                    break;
                
                default:
                    printf("Unsupported path type");
                    return -1;
                    break;
            }

            dyn_controller->define_moveTo_follow_path_task(std::vector<bool>{control_dims[0], control_dims[1], control_dims[2], // Linear
                                                                             control_dims[3], control_dims[4], control_dims[5]},// Angular
                                                            tube_path_points,
                                                            tube_tolerances,
                                                            tube_speed,
                                                            1.0, 0.1, //contact_threshold linear and angular
                                                            task_time_limit_sec,// time_limit
                                                            control_null_space,
                                                            desired_null_space_angle,
                                                            path_poses); // TF pose
            break;

        case task_model::moveTo:
            dyn_controller->define_moveTo_task(std::vector<bool>{control_dims[0], control_dims[1], control_dims[2], // Linear
                                                                 control_dims[3], control_dims[4], control_dims[5]},// Angular
                                               tube_start_position,
                                               tube_tolerances,
                                               tube_speed,
                                               1.0, 0.1, //contact_threshold linear and angular
                                               task_time_limit_sec,// time_limit
                                               control_null_space,
                                               desired_null_space_angle,
                                               desired_ee_pose); // TF pose
            break;

        case task_model::moveGuarded:
            dyn_controller->define_moveGuarded_task(std::vector<bool>{control_dims[0], control_dims[1], control_dims[2], // Linear
                                                                      control_dims[3], control_dims[4], control_dims[5]},// Angular
                                                    tube_start_position,
                                                    tube_tolerances,
                                                    tube_speed,
                                                    1.0, 0.1, //contact_threshold linear and angular
                                                    task_time_limit_sec,// time_limit
                                                    control_null_space,
                                                    desired_null_space_angle,
                                                    desired_ee_pose); // TF pose
            break;

        case task_model::moveTo_weight_compensation:
            dyn_controller->define_moveTo_weight_compensation_task(std::vector<bool>{control_dims[0], control_dims[1], control_dims[2], // Linear
                                                                                     control_dims[3], control_dims[4], control_dims[5]},// Angular
                                                                   tube_start_position,
                                                                   tube_tolerances,
                                                                   tube_speed,
                                                                   1.0, 0.1, // contact_threshold linear and angular
                                                                   task_time_limit_sec,// time_limit
                                                                   control_null_space,
                                                                   desired_null_space_angle,
                                                                   use_mass_alternation,
                                                                   desired_ee_pose); // TF pose
            break;

        case task_model::full_pose:
            dyn_controller->define_full_pose_task(std::vector<bool>{control_dims[0], control_dims[1], control_dims[2], // Linear
                                                                    control_dims[3], control_dims[4], control_dims[5]}, // Angular
                                                  desired_ee_pose,
                                                  1.0, 0.2, //contact_threshold linear and angular
                                                  task_time_limit_sec,
                                                  control_null_space,
                                                  desired_null_space_angle,
                                                  tube_tolerances[7]); // Null space tolerance
            break;

        case task_model::gravity_compensation:
            dyn_controller->define_gravity_compensation_task(task_time_limit_sec);
            break;

        default:
            assert(("Unsupported task model", false));
            return -1;
            break;
    }

    return 0;
}

void run_test(kinova_mediator &robot_driver)
{
    double time_duration = 20.0f; // Duration of the example (seconds)
    int timer_count = 0;
    int64_t now = 0;
    int64_t last = 0;
    int id_solver_result = 0;
    
    KDL::JntArray jnt_array_command(7), jnt_array_feedback(7), jnt_array_feedback_2(7), jnt_array_feedback_3(7);
    const KDL::JntArray ZERO_JOINT_ARRAY(7);

    KDL::Chain robot_chain = robot_driver.get_robot_model();
    KDL::Wrenches zero_wrenches(robot_chain.getNrOfSegments(), KDL::Wrench::Zero());

    std::shared_ptr<KDL::Solver_RNE> id_solver = std::make_shared<KDL::Solver_RNE>(robot_chain, KDL::Vector(0.0, 0.0, -9.81289), robot_driver.get_joint_inertia(), robot_driver.get_joint_torque_limits(), true);

    // printf("Test run started\n");
    int return_flag = 0;
    int iteration_count = 0;

    // Real-time loop
    while (timer_count < (time_duration * 1000))
    {
        now = GetTickUs();

        if (now - last > 1000)
        {
            robot_driver.get_joint_state(jnt_array_feedback, jnt_array_feedback_2, jnt_array_feedback_3);
            // std::cout << "Pos: " << jnt_array_feedback << std::endl;
            // std::cout << "Vel: " << jnt_array_feedback_2 << std::endl;
            // std::cout << "Torque: " << jnt_array_feedback_3 << std::endl;
            // std::cout << std::endl;

            id_solver_result = id_solver->CartToJnt(jnt_array_feedback, ZERO_JOINT_ARRAY, ZERO_JOINT_ARRAY, zero_wrenches, jnt_array_command);
            if (id_solver_result != 0) return;

            if (iteration_count == 0)
            {
                if (robot_driver.set_control_mode(control_mode::TORQUE) == -1)
                {
                    printf("Incorrect control mode\n");
                    return;
                }
            }

            return_flag = robot_driver.set_joint_torques(jnt_array_command);
            // std::cout <<  "Torque command: "<< jnt_array_command << std::endl;

            if (return_flag == -1)
            {
                robot_driver.stop_robot_motion();
                printf("Robot stoped: error in control\n");
                return;
            }

            timer_count++;
            last = GetTickUs();
            iteration_count++;
        }
    }

    robot_driver.stop_robot_motion();
    printf("Task completed\n");
}

int run_main_control(kinova_mediator &robot_driver)
{
    const double DT_SEC = 1.0 / static_cast<double>(RATE_HZ);
    const int DT_MICRO = SECOND / RATE_HZ;
    const int DT_STOPPING_MICRO = SECOND / dynamics_parameter::STOPPING_MOTION_LOOP_FREQ;

    KDL::Chain robot_chain = robot_driver.get_robot_model();

    // Constraint comming from the Vereshchagin HD solver
    assert(JOINTS == robot_driver.get_robot_model().getNrOfSegments());

    if (robot_driver.stop_robot_motion() == -1)
    {
        printf("Error in stopping the robot\n");
        return -1;
    }

    dynamics_controller controller(&robot_driver, RATE_HZ, compensate_gravity);

    int return_flag = define_task(&controller); 
    if (return_flag != 0)
    {
        printf("Error in defining task for robot\n");
        return -1;
    }

    if (desired_task_model == task_model::full_pose) 
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                    max_command, error_alpha,
                                    bias_threshold, bias_step, gain_threshold,
                                    gain_step, min_bias_sat, min_command_sat,
                                    null_space_abag_parameters, compensation_parameters,
                                    STOP_MOTION_ERROR_ALPHA,
                                    STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                    STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);
    }
    else if (desired_task_model == task_model::moveGuarded)
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                    max_command, error_alpha_1,
                                    bias_threshold_1, bias_step_1, gain_threshold_1,
                                    gain_step_1, min_bias_sat, min_command_sat,
                                    null_space_abag_parameters, compensation_parameters,
                                    STOP_MOTION_ERROR_ALPHA,
                                    STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                    STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);
    }
    else if (desired_task_model == task_model::moveTo_follow_path)
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                    max_command, error_alpha_3,
                                    bias_threshold_3, bias_step_3, gain_threshold_3,
                                    gain_step_3, min_bias_sat, min_command_sat,
                                    null_space_abag_parameters, compensation_parameters,
                                    STOP_MOTION_ERROR_ALPHA,
                                    STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                    STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);

    }
    else if (desired_task_model == task_model::moveTo_weight_compensation)
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                    max_command, error_alpha_4,
                                    bias_threshold_4, bias_step_4, gain_threshold_4,
                                    gain_step_4, min_bias_sat, min_command_sat,
                                    null_space_abag_parameters, compensation_parameters,
                                    STOP_MOTION_ERROR_ALPHA,
                                    STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                    STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);
    }
    else if (desired_task_model == task_model::moveTo)
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                    max_command, error_alpha_2,
                                    bias_threshold_2, bias_step_2, gain_threshold_2,
                                    gain_step_2, min_bias_sat, min_command_sat,
                                    null_space_abag_parameters, compensation_parameters,
                                    STOP_MOTION_ERROR_ALPHA,
                                    STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                    STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);
    }

    else if (desired_task_model == task_model::gravity_compensation)
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                    max_command, error_alpha,
                                    bias_threshold, bias_step, gain_threshold,
                                    gain_step, min_bias_sat, min_command_sat,
                                    null_space_abag_parameters, compensation_parameters,
                                    STOP_MOTION_ERROR_ALPHA,
                                    STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                    STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);
    }
    else
    {
        printf("Error in setting the parameters\n");
        return -1;
    }

    return_flag = controller.initialize(desired_control_mode, desired_dynamics_interface, log_data, motion_profile_id);
    if (return_flag != 0)
    {
        printf("Error in intializing the arm\n");
        return -1;
    }

    KDL::JntArray torque_command(7), joint_pos(7), joint_vel(7), joint_torque(7);
    KDL::Wrenches wrenches(robot_chain.getNrOfSegments(), KDL::Wrench::Zero());

    // double loop_time = 0.0;
    double total_time_sec = 0.0;
    int loop_iteration_count = 0;
    int stop_loop_iteration_count = 0;
    bool stopping_sequence_on = false;
    bool trigger_stopping_sequence = false;
    control_loop_delay_count = 0;
    return_flag = 0;

    // Real-time loop
    while (1)
    {
        // Save current time point
        loop_start_time = std::chrono::steady_clock::now();
        if (!stopping_sequence_on) total_time_sec = loop_iteration_count * DT_SEC;

        //Get current robot state from the joint sensors: velocities and angles
        robot_driver.get_joint_state(joint_pos, joint_vel, joint_torque);

        // Make one control iteration (step) -> Update control commands
        return_flag = controller.step(joint_pos, joint_vel, wrenches[robot_chain.getNrOfSegments()- 1], torque_command.data, total_time_sec, loop_iteration_count, stop_loop_iteration_count, stopping_sequence_on);
        if (return_flag == -1) trigger_stopping_sequence = true;

        if (stopping_sequence_on) // Robot will be controlled to stop its motion and eventually lock
        {
            if (return_flag == 1) // Stop motion task completed
            {
                // Make sure that the robot is locked (freezed)
                controller.engage_lock();

                stopping_sequence_on = false;
                total_time_sec += (double)stop_loop_iteration_count / dynamics_parameter::STOPPING_MOTION_LOOP_FREQ;
                printf("Robot stopped!\n");
                controller.deinitialize();
                return 0;
            }

            // Apply joint commands using torque control interface (bypass all safety checks)
            if (controller.apply_joint_control_commands(true) == -1)
            {
                // Make sure that the robot is locked (freezed)
                controller.engage_lock();

                stopping_sequence_on = false;
                total_time_sec += (double)stop_loop_iteration_count / dynamics_parameter::STOPPING_MOTION_LOOP_FREQ;
                printf("Robot stopped!\n");
                controller.deinitialize();
                return -1;
            }

            stop_loop_iteration_count++;
            if (enforce_loop_frequency(DT_STOPPING_MICRO) != 0) control_loop_delay_count++;

            // Testing loop time
            // loop_time += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - loop_start_time).count();
            // if (stop_loop_iteration_count == 500) 
            // {
            //     printf("Stop loop time: %f\n", loop_time / 500.0);
            //     robot_driver.stop_robot_motion();
            //     controller.deinitialize();
            //     return 0;
            // }
        }
        else // Nominal task execution mode
        {
            if (!trigger_stopping_sequence)
            {
                // Apply joint commands using safe control interface
                if (controller.apply_joint_control_commands(false) == -1) trigger_stopping_sequence = true;
            }

            if (trigger_stopping_sequence)
            {
                trigger_stopping_sequence = false;
                stopping_sequence_on = true;
                stop_loop_iteration_count = 0;

                printf("Stopping behaviour triggered!\n");
                continue;
            }

            loop_iteration_count++;
            if (enforce_loop_frequency(DT_MICRO) != 0) control_loop_delay_count++;

            // Testing loop time
            // loop_time += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - loop_start_time).count();
            // if (loop_iteration_count == 2000) 
            // {
            //     printf("Main loop time: %f\n", loop_time / 2000.0);
            //     trigger_stopping_sequence = true;
            //     controller.deinitialize();
            //     return 0;
            // }
        }
    }

    robot_driver.stop_robot_motion();
    controller.deinitialize();
    printf("Task completed\n");
    return 0;
}

int main(int argc, char **argv)
{
    // printf("kinova MAIN Started \n");
    RATE_HZ              = 500; // Loop frequency in Hz
    control_dims         = std::vector<bool>{true, true, true, // Linear
                                             false, false, false}; // Angular
    tube_tolerances      = std::vector<double>{0.01, 0.02, 0.02,
                                               0.0, 0.0, 0.0,
                                               0.001, 0.0}; // Last tolerance is in unit of degrees - Null-space tolerance
    environment          = kinova_environment::REAL;
    robot_model_id       = kinova_model::URDF;
    id                   = robot_id::KINOVA_GEN3_2;
    desired_pose_id      = desired_pose::HOME;
    desired_control_mode = control_mode::TORQUE;
    desired_task_model   = task_model::moveTo;
    path_type            = path_types::SINE_PATH;
    motion_profile_id    = m_profile::CONSTANT;
    task_time_limit_sec  = 5.5;
    tube_speed           = 0.01;
    compensate_gravity   = true;
    control_null_space   = false;
    use_mass_alternation = false;
    log_data             = false;

    kinova_mediator robot_driver;
    int return_flag = 0;
    if      (desired_pose_id == desired_pose::HOME)      return_flag = go_to(robot_driver, desired_pose::HOME);
    else if (desired_pose_id == desired_pose::CANDLE)    return_flag = go_to(robot_driver, desired_pose::CANDLE);
    else if (desired_pose_id == desired_pose::RETRACT)   return_flag = go_to(robot_driver, desired_pose::RETRACT);
    else if (desired_pose_id == desired_pose::PACKAGING) return_flag = go_to(robot_driver, desired_pose::PACKAGING);
    else return 0;

    if (return_flag != 0) return 0;

    // Extract robot model and if not simulation, establish connection with motor drivers
    if (!robot_driver.is_initialized()) robot_driver.initialize(robot_model_id, environment, id);
    if (!robot_driver.is_initialized())
    {
        printf("Robot is not initialized\n");
        return 0;
    }

    assert(JOINTS == robot_driver.get_robot_model().getNrOfSegments());
    // if (robot_driver.stop_robot_motion() == -1) return 0;

    // run_test(robot_driver); return 0;

    if (run_main_control(robot_driver) == -1) return 0;
    return 0;

    dynamics_controller controller(&robot_driver, RATE_HZ, compensate_gravity);

    int initial_result = define_task(&controller);
    if (initial_result != 0) return -1;

    if (desired_task_model == task_model::full_pose) 
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                  max_command, error_alpha,
                                  bias_threshold, bias_step, gain_threshold,
                                  gain_step, min_bias_sat, min_command_sat,
                                  null_space_abag_parameters, compensation_parameters,
                                  STOP_MOTION_ERROR_ALPHA,
                                  STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                  STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);
    }
    else if (desired_task_model == task_model::moveGuarded)
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                  max_command, error_alpha_1,
                                  bias_threshold_1, bias_step_1, gain_threshold_1,
                                  gain_step_1, min_bias_sat, min_command_sat,
                                  null_space_abag_parameters, compensation_parameters,
                                  STOP_MOTION_ERROR_ALPHA,
                                  STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                  STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);
    }
    else if (desired_task_model == task_model::moveTo_follow_path)
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                  max_command, error_alpha_3,
                                  bias_threshold_3, bias_step_3, gain_threshold_3,
                                  gain_step_3, min_bias_sat, min_command_sat,
                                  null_space_abag_parameters, compensation_parameters,
                                  STOP_MOTION_ERROR_ALPHA,
                                  STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                  STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);
    }
    else if (desired_task_model == task_model::moveTo_weight_compensation)
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                  max_command, error_alpha_4,
                                  bias_threshold_4, bias_step_4, gain_threshold_4,
                                  gain_step_4, min_bias_sat, min_command_sat,
                                  null_space_abag_parameters, compensation_parameters,
                                  STOP_MOTION_ERROR_ALPHA,
                                  STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                  STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);
    }
    else if (desired_task_model == task_model::moveTo)
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                  max_command, error_alpha_2,
                                  bias_threshold_2, bias_step_2, gain_threshold_2,
                                  gain_step_2, min_bias_sat, min_command_sat,
                                  null_space_abag_parameters, compensation_parameters,
                                  STOP_MOTION_ERROR_ALPHA,
                                  STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                  STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);
    }

    else if (desired_task_model == task_model::gravity_compensation)
    {
        controller.set_parameters(time_horizon_amplitude, abag_error_type, 
                                  max_command, error_alpha,
                                  bias_threshold, bias_step, gain_threshold,
                                  gain_step, min_bias_sat, min_command_sat,
                                  null_space_abag_parameters, compensation_parameters,
                                  STOP_MOTION_ERROR_ALPHA,
                                  STOP_MOTION_BIAS_THRESHOLD, STOP_MOTION_BIAS_STEP,
                                  STOP_MOTION_GAIN_THRESHOLD, STOP_MOTION_GAIN_STEP);
    }
    else return 0;

    initial_result = controller.initialize(desired_control_mode, desired_dynamics_interface, log_data, motion_profile_id);
    if (initial_result != 0) return -1;

    controller.control();
    robot_driver.stop_robot_motion();

    controller.deinitialize();
    robot_driver.deinitialize();

    return_flag = go_to(robot_driver, desired_pose::RETRACT);
    return 0;
}