#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>

#include "serialPort/SerialPort.h"
#include "unitreeMotor/unitreeMotor.h"

#include <string>

static constexpr double GEAR_RATIO  = 6.33;
static constexpr double DEFAULT_KW  = 0.02;
static constexpr int    LEFT_ID     = 0;
static constexpr int    RIGHT_ID    = 1;

class MotorInterfaceNode : public rclcpp::Node
{
public:
  MotorInterfaceNode()
  : Node("motor_interface")
  {
    declare_parameter<double>("gear_ratio", GEAR_RATIO);
    declare_parameter<double>("k_w",        DEFAULT_KW);
    declare_parameter<int>   ("left_id",    LEFT_ID);
    declare_parameter<int>   ("right_id",   RIGHT_ID);
    declare_parameter<bool>  ("dry_run",    false);

    declare_parameter<std::string>("lhs_port", "/dev/ttyUSB0");
    declare_parameter<std::string>("rhs_port", "/dev/ttyUSB1");

    gear_ratio_ = get_parameter("gear_ratio").as_double();
    k_w_        = get_parameter("k_w").as_double();
    left_id_    = get_parameter("left_id").as_int();
    right_id_   = get_parameter("right_id").as_int();
    dry_run_    = get_parameter("dry_run").as_bool();

    serial_port_lhs_ = new SerialPort(get_parameter("lhs_port").as_string());
    serial_port_rhs_ = new SerialPort(get_parameter("rhs_port").as_string());

    left_sub_ = create_subscription<std_msgs::msg::Float64>(
      "left_motor_speed", 10,
      std::bind(&MotorInterfaceNode::left_cb, this, std::placeholders::_1));

    right_sub_ = create_subscription<std_msgs::msg::Float64>(
      "right_motor_speed", 10,
      std::bind(&MotorInterfaceNode::right_cb, this, std::placeholders::_1));
  }

  // ── Pure maths -- public so the unit test can call it directly ─────────────
  //
  // Converts an output-shaft speed [rad/s] into the MotorCmdValues that
  // implement speed mode of the hybrid control law:
  //
  //   tau = tau_ff + k_p*(p_des - p) + k_d*(w_des - w)
  //
  // Speed mode: tau_ff=0, p_des=0, k_p=0  =>  tau = k_d*(w_des - w)
  // The motor firmware closes the loop; we just supply w_des and k_d.
  MotorCmd build_speed_cmd(int motor_id, double omega_output_shaft) {

     MotorCmd cmd;

     cmd.motorType = MotorType::GO_M8010_6;
     cmd.mode = queryMotorMode(MotorType::GO_M8010_6,MotorMode::FOC);
     cmd.id   = motor_id;
     cmd.kp   = 0.0;
     cmd.kd   = 0.01;
     cmd.q    = 0.0;
     cmd.dq   = omega_output_shaft*queryGearRatio(MotorType::GO_M8010_6);
     cmd.tau  = 0.0;

    return cmd;
  }

private:

#define LEFT  0
#define RIGHT 1

  // ── Dispatch ──────────────────────────────────────────────────────────────
  void send_speed_cmd(int motor_id, double omega_output_shaft, int side) {
    MotorCmd cmd = build_speed_cmd(motor_id, omega_output_shaft);

    MotorData hw_data;
    hw_data.motorType = MotorType::GO_M8010_6;
    if (side == LEFT)
       serial_port_lhs_->sendRecv(&cmd, &hw_data);
    else
       serial_port_rhs_->sendRecv(&cmd, &hw_data);


    RCLCPP_DEBUG(get_logger(),
      "Motor %d | cmd_W=%.3f rad/s (rotor) | fb_pos=%.3f fb_spd=%.3f "
      "fb_tau=%.3f temp=%dC err=%d",
      motor_id, cmd.dq,
      hw_data.q, hw_data.dq, hw_data.tau, hw_data.temp, hw_data.merror);

    if (hw_data.merror != 0) {
      RCLCPP_WARN(get_logger(),
        "Motor %d error %d (1=overheat 2=overcurrent 3=overvolt 4=encoder)",
        motor_id, hw_data.merror);
    }
  }

  // ── Callbacks ─────────────────────────────────────────────────────────────
  void left_cb (const std_msgs::msg::Float64::SharedPtr msg)
  { send_speed_cmd(left_id_,  msg->data, LEFT); }

  void right_cb(const std_msgs::msg::Float64::SharedPtr msg)
  { send_speed_cmd(right_id_, msg->data, RIGHT); }

  // ── Members ───────────────────────────────────────────────────────────────
  double gear_ratio_;
  double k_w_;
  int    left_id_;
  int    right_id_;
  bool   dry_run_;

  rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr debug_pub_;
  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr        left_sub_;
  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr        right_sub_;

  SerialPort * serial_port_lhs_;
  SerialPort * serial_port_rhs_;
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MotorInterfaceNode>());
  rclcpp::shutdown();
  return 0;
}
