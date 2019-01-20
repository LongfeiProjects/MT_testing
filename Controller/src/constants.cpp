#include <constants.hpp>

#define PI 3.14159265358979323846
#define DEG_TO_RAD(x) (x) * PI / 180.0

namespace youbot_constants
{
    //Robot ID/Name
    const std::string ID("yb_arm");

    // Number of joints in the manipulator
    const int NUMBER_OF_JOINTS(5);
    const int NUMBER_OF_SEGMENTS(5);
    const int NUMBER_OF_FRAMES(6);

    //Arm's root acceleration
    const std::vector<double> root_acceleration {0.0, 0.0, 9.81, 0.0, 0.0, 0.0};

    //Kuka youBot store position limit values: positive and negative
    const std::vector<double> joint_position_limits_max_1 {  2.9496,  1.5707,  2.5481,  1.7889,  2.9234};
    const std::vector<double> joint_position_limits_min_1 { -2.9496, -1.1344, -2.6354, -1.7889, -2.9234};

    //Position limit values from URDF file: positive and negative
    const std::vector<double> joint_position_limits_max_2 {5.89921, 2.70526,  0.00000, 3.57792, 5.84685};
    const std::vector<double> joint_position_limits_min_2 {0.00000, 0.00000, -5.16617, 0.00000, 0.00000};
    // const std::vector<double> joint_position_limits_max_2 { 5.899210,  2.705260,  0.000001,  3.577920,  5.846850};
    // const std::vector<double> joint_position_limits_min_2 {-0.000001, -0.000001, -5.166170, -0.000001, -0.000001};
    
    const std::vector<double> joint_position_limits_max_2_sim { 5.899210,  2.705260,  0.000001,  3.577920,  5.846850};
    const std::vector<double> joint_position_limits_min_2_sim {-0.000001, -0.000001, -5.166170, -0.000001, -0.000001};

    const std::vector<double> joint_position_thresholds {DEG_TO_RAD(0), DEG_TO_RAD(0), DEG_TO_RAD(0), DEG_TO_RAD(0), DEG_TO_RAD(0)};
    // const std::vector<double> joint_position_thresholds {DEG_TO_RAD(10), DEG_TO_RAD(10), DEG_TO_RAD(10), DEG_TO_RAD(8), DEG_TO_RAD(5)};

    // Robocup URDF file parameters for velocity limits
    // std::vector<double> joint_velocity_limits {1.5707, 0.8, 1.0, 1.5707, 1.5707};

    // YouBot Store velocity limits
    const std::vector<double> joint_velocity_limits {1.5707, 01.5707, 1.5707, 1.5707, 1.5707};

    // JP's max torques
    // std::vector<double> joint_torque_limits {12.9012, 12.9012, 8.2700, 4.1748, 1.7550};

    // youBot store's max torques 
    const std::vector<double> joint_torque_limits {9.5, 9.5, 6.0, 2.0, 1.0};

    // Benjamin Keiser' max torques (fast version)
    // std::vector<double> joint_torque_limits {17.0, 17.0, 8.0, 2.7, 1.0}};

    // Offsets required for the youBot store model: Negative Candle config values -Robocup
    // std::vector<double> youbot_joint_offsets {-2.1642, -1.13446, 2.54818, -1.78896, -2.9234};

    // Offsets required for the youBot store model: Negative Candle config values - JP 
    const std::vector<double> joint_offsets {-2.94960, -1.13446, 2.54818, -1.78896, -2.9234};

    // Offsets required for the youBot store model: Negative Candle config values -Sven's
    // std::vector<double> joint_offsets {-2.9496, -1.1344, 2.6354, -1.7890, -2.9234};

    // Offsets required for the youBot store model: Negative Candle config values: keiser's
    // std::vector<double> joint_offsets {-2.9496, -1.1344, 2.5481, -1.7889, -2.9234};

    // Rotor inertia - "d" in the algorithm: Computed from youBot store values
    const std::vector<double> joint_inertia {0.33848, 0.33848, 0.13571, 0.04698, 0.01799};

    const std::string config_path = "/home/djole/Master/Thesis/GIT/MT_testing/youbot_driver/config";
    
    // const std::string urdf_path = "/home/djole/Master/Thesis/GIT/MT_testing/Controller/urdf/youbot_arm_only.urdf";
    const std::string urdf_path = "/home/djole/Master/Thesis/GIT/MT_testing/Controller/urdf/youbot_arm_zero_inertia.urdf";
    // const std::string urdf_path = "/home/djole/Master/Thesis/GIT/MT_testing/Controller/urdf/youbot_arm_zero_inertia_new_mass.urdf";

    const std::string root_name = "arm_link_0";
    const std::string tooltip_name = "arm_link_5";
}

namespace abag_parameter
{
    // How many dimension ABAG controller is supposed to control
    const int DIMENSIONS(6);
    const bool REVERSE_ERROR(true);
    const bool USE_ERROR_MAGNITUDE(false); //Experimental feature! False is normal!

    const Eigen::VectorXd ERROR_ALPHA = (Eigen::VectorXd(DIMENSIONS) \
                                     << 0.178001, 0.178001, 0.150000, 
                                        0.178001, 0.178001, 0.178001).finished();

    // Bias parameters
    const Eigen::VectorXd BIAS_THRESHOLD = (Eigen::VectorXd(DIMENSIONS) \
                                     << 0.724277, 0.724277, 0.000007, 
                                        0.724277, 0.724277, 0.724277).finished();

    const Eigen::VectorXd BIAS_STEP = (Eigen::VectorXd(DIMENSIONS) \
                                     << 0.503495, 0.503495, 0.013495, 
                                        0.503495, 0.503495, 0.503495).finished();


    // Gain parameters
    const Eigen::VectorXd GAIN_THRESHOLD = (Eigen::VectorXd(DIMENSIONS) \
                                     << 0.432492, 0.432492, 0.752492, 
                                        0.432492, 0.432492, 0.432492).finished();

    const Eigen::VectorXd GAIN_STEP = (Eigen::VectorXd(DIMENSIONS) \
                                     << 0.655152, 0.655152, 0.025152, 
                                        0.655152, 0.655152, 0.655152).finished();

    // Saturation limits   
    const Eigen::VectorXd MIN_BIAS_SAT_LIMIT = (Eigen::VectorXd(DIMENSIONS) \
                                           << -1.0, -1.0, -1.0, 
                                              -1.0, -1.0, -1.0).finished();

    const Eigen::VectorXd MAX_BIAS_SAT_LIMIT = (Eigen::VectorXd(DIMENSIONS) \
                                           << 1.0, 1.0, 1.0, 
                                              1.0, 1.0, 1.0).finished();


    const Eigen::VectorXd MIN_GAIN_SAT_LIMIT = (Eigen::VectorXd(DIMENSIONS) \
                                           << 0.0, 0.0, 0.0, 
                                              0.0, 0.0, 0.0).finished();

    const Eigen::VectorXd MAX_GAIN_SAT_LIMIT = (Eigen::VectorXd(DIMENSIONS) \
                                           << 1.0, 1.0, 1.0, 
                                              1.0, 1.0, 1.0).finished();


    const Eigen::VectorXd MIN_COMMAND_SAT_LIMIT = (Eigen::VectorXd(DIMENSIONS) \
                                           << -1.0, -1.0, -1.0, 
                                              -1.0, -1.0, -1.0).finished();

    const Eigen::VectorXd MAX_COMMAND_SAT_LIMIT = (Eigen::VectorXd(DIMENSIONS) \
                                           << 1.0, 1.0, 1.0, 
                                              1.0, 1.0, 1.0).finished();
}

namespace dynamics_parameter
{
    // Number of task constraints imposed on the robot, i.e. Cartesian DOFS
    const int NUMBER_OF_CONSTRAINTS(6);
    const Eigen::IOFormat WRITE_FORMAT(6, Eigen::DontAlignCols, " ", "", "", "\n");
    const std::string LOG_FILE_PATH("/home/djole/Master/Thesis/GIT/MT_testing/Controller/visualization/control_error.txt");
}