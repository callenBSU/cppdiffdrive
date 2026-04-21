#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <random>

static constexpr double LIN_MIN      = -0.5;
static constexpr double LIN_MAX      =  1.5;
static constexpr double ANG_MIN      = -1.0;
static constexpr double ANG_MAX      =  1.0;
static constexpr double INTERVAL     =  2.0;   // seconds between new targets
static constexpr double PUBLISH_RATE = 10.0;   // Hz

class CmdVelPublisher : public rclcpp::Node
{
public:
  CmdVelPublisher()
  : Node("cmd_vel_publisher"),
    rng_(std::random_device{}()),
    lin_dist_(LIN_MIN, LIN_MAX),
    ang_dist_(ANG_MIN, ANG_MAX)
  {
    pub_ = create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);

    pick_new_target(); // initialise immediately

    publish_timer_ = create_wall_timer(
      std::chrono::duration<double>(1.0 / PUBLISH_RATE),
      std::bind(&CmdVelPublisher::publish_cb, this));

    target_timer_ = create_wall_timer(
      std::chrono::duration<double>(INTERVAL),
      std::bind(&CmdVelPublisher::pick_new_target, this));

    RCLCPP_INFO(get_logger(),
      "cmd_vel_publisher on | pub_rate=%.1f Hz | target_change_interval=%.1f s",
      PUBLISH_RATE, INTERVAL);
  }

private:
  // Round a double to 3 decimal places (mirrors Python's round(x, 3))
  static double round3(double v)
  {
    return std::round(v * 1000.0) / 1000.0;
  }

  void pick_new_target()
  {
    target_linear_  = round3(lin_dist_(rng_));
    target_angular_ = round3(ang_dist_(rng_));

    RCLCPP_INFO(get_logger(),
      "New target -> linear.x=%.3f m/s | angular.z=%.3f rad/s",
      target_linear_, target_angular_);
  }

  void publish_cb()
  {
    geometry_msgs::msg::Twist msg;
    msg.linear.x  = target_linear_;
    msg.angular.z = target_angular_;  // BUG FIX: was msg.linear.z
    pub_->publish(msg);
  }

  // RNG
  std::mt19937                          rng_;
  std::uniform_real_distribution<double> lin_dist_;
  std::uniform_real_distribution<double> ang_dist_;

  // State
  double target_linear_  {0.0};
  double target_angular_ {0.0};

  // ROS handles
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr publish_timer_;
  rclcpp::TimerBase::SharedPtr target_timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CmdVelPublisher>());
  rclcpp::shutdown();
  return 0;
}
