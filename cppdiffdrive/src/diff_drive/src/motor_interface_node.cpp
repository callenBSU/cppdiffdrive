#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>

// Only pull in the Unitree SDK when we are doing a real hardware build.
// Define UNITREE_SDK_AVAILABLE in CMakeLists when the SDK is present.
#ifdef UNITREE_SDK_AVAILABLE
  #include "unitreeMotor/unitreeMotor.h"
  #include "serialPort/SerialPort.h"
#endif

static constexpr double GEAR_RATIO  = 6.33;
static constexpr double DEFAULT_KW  = 0.02;
static constexpr int    LEFT_ID     = 0;
static constexpr int    RIGHT_ID    = 1;

// ── Computed command (hardware-agnostic) ─────────────────────────────────────
// Mirrors the fields of MotorCmd that we actually fill.
struct MotorCmdValues
{
  int    id;
  int    mode;   // always 1 (FOC) in our use case
  double T;      // feedforward torque  [N.m]
  double W;      // rotor target speed  [rad/s]  (already x gear_ratio)
  double Pos;    // rotor target pos    [rad]
  double K_P;    // position stiffness
  double K_W;    // velocity damping
};

// ─────────────────────────────────────────────────────────────────────────────

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

    gear_ratio_ = get_parameter("gear_ratio").as_double();
    k_w_        = get_parameter("k_w").as_double();
    left_id_    = get_parameter("left_id").as_int();
    right_id_   = get_parameter("right_id").as_int();
    dry_run_    = get_parameter("dry_run").as_bool();

    if (dry_run_) {
      RCLCPP_WARN(get_logger(),
        "DRY-RUN MODE -- no serial port opened, commands published to /motor_cmd_debug");
      // Publishes a Float64MultiArray per motor command:
      // [id, mode, T, W (rotor), Pos, K_P, K_W]
      debug_pub_ = create_publisher<std_msgs::msg::Float64MultiArray>(
        "motor_cmd_debug", 10);
    } else {
#ifdef UNITREE_SDK_AVAILABLE
      serial_port_ = std::make_unique<SerialPort>("/dev/ttyUSB0");
      RCLCPP_INFO(get_logger(), "Hardware mode -- serial port opened.");
#else
      RCLCPP_FATAL(get_logger(),
        "Hardware mode requested but UNITREE_SDK_AVAILABLE is not defined. "
        "Rebuild with the SDK, or launch with dry_run:=true.");
      throw std::runtime_error("SDK unavailable in hardware mode");
#endif
    }

    RCLCPP_INFO(get_logger(),
      "MotorInterface | left_id=%d right_id=%d gear_ratio=%.2f k_w=%.3f dry_run=%s",
      left_id_, right_id_, gear_ratio_, k_w_, dry_run_ ? "true" : "false");

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
  MotorCmdValues build_speed_cmd(int motor_id, double omega_output_shaft) const
  {
    MotorCmdValues cmd;
    cmd.id   = motor_id;
    cmd.mode = 1;                                // FOC closed-loop
    cmd.T    = 0.0;                              // no feedforward torque
    cmd.Pos  = 0.0;                              // position control disabled
    cmd.K_P  = 0.0;                              // position stiffness = 0
    cmd.W    = omega_output_shaft * gear_ratio_; // rotor speed [rad/s]
    cmd.K_W  = k_w_;                             // velocity damping
    return cmd;
  }

private:
  // ── Dispatch ──────────────────────────────────────────────────────────────
  void send_speed_cmd(int motor_id, double omega_output_shaft)
  {
    const MotorCmdValues cmd = build_speed_cmd(motor_id, omega_output_shaft);

    if (dry_run_) {
      publish_debug(cmd);
      return;
    }

#ifdef UNITREE_SDK_AVAILABLE
    MotorCmd  hw_cmd;
    MotorData hw_data;
    hw_cmd.motorType = MotorType::Go2;
    hw_cmd.id   = cmd.id;
    hw_cmd.mode = cmd.mode;
    hw_cmd.T    = cmd.T;
    hw_cmd.W    = cmd.W;
    hw_cmd.Pos  = cmd.Pos;
    hw_cmd.K_P  = cmd.K_P;
    hw_cmd.K_W  = cmd.K_W;
    serial_port_->sendRecv(&hw_cmd, &hw_data);

    RCLCPP_DEBUG(get_logger(),
      "Motor %d | cmd_W=%.3f rad/s (rotor) | fb_pos=%.3f fb_spd=%.3f "
      "fb_tau=%.3f temp=%dC err=%d",
      motor_id, cmd.W,
      hw_data.Pos, hw_data.W, hw_data.T, hw_data.Temp, hw_data.MError);

    if (hw_data.MError != 0) {
      RCLCPP_WARN(get_logger(),
        "Motor %d error %d (1=overheat 2=overcurrent 3=overvolt 4=encoder)",
        motor_id, hw_data.MError);
    }
#endif
  }

  // ── Debug publisher ───────────────────────────────────────────────────────
  // Layout: [id, mode, T, W (rotor rad/s), Pos, K_P, K_W]
  void publish_debug(const MotorCmdValues & cmd)
  {
    std_msgs::msg::Float64MultiArray msg;
    msg.data = {
      static_cast<double>(cmd.id),
      static_cast<double>(cmd.mode),
      cmd.T,
      cmd.W,
      cmd.Pos,
      cmd.K_P,
      cmd.K_W
    };
    debug_pub_->publish(msg);

    RCLCPP_INFO(get_logger(),
      "[DRY-RUN] Motor %d | mode=%d T=%.3f W(rotor)=%.3f Pos=%.3f K_P=%.3f K_W=%.3f",
      cmd.id, cmd.mode, cmd.T, cmd.W, cmd.Pos, cmd.K_P, cmd.K_W);
  }

  // ── Callbacks ─────────────────────────────────────────────────────────────
  void left_cb (const std_msgs::msg::Float64::SharedPtr msg)
  { send_speed_cmd(left_id_,  msg->data); }

  void right_cb(const std_msgs::msg::Float64::SharedPtr msg)
  { send_speed_cmd(right_id_, msg->data); }

  // ── Members ───────────────────────────────────────────────────────────────
  double gear_ratio_;
  double k_w_;
  int    left_id_;
  int    right_id_;
  bool   dry_run_;

  rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr debug_pub_;
  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr        left_sub_;
  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr        right_sub_;

#ifdef UNITREE_SDK_AVAILABLE
  std::unique_ptr<SerialPort> serial_port_;
#endif
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MotorInterfaceNode>());
  rclcpp::shutdown();
  return 0;
}
