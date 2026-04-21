#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/float64.hpp>

class DiffDriveNode : public rclcpp::Node
{
public:
  DiffDriveNode() : Node("diff_drive")
  {
    declare_parameter<double>("wheel_seperation", 0.5); // b of equation
    declare_parameter<double>("wheel_radius",     0.1); // r of equation

    b_ = get_parameter("wheel_seperation").as_double();
    r_ = get_parameter("wheel_radius").as_double();

    RCLCPP_INFO(get_logger(),
      "DiffDrive on | wheel_seperation = %.3f m | wheel_radius = %.3f m", b_, r_);

    left_pub_  = create_publisher<std_msgs::msg::Float64>("left_motor_speed",  10);
    right_pub_ = create_publisher<std_msgs::msg::Float64>("right_motor_speed", 10);

    sub_ = create_subscription<geometry_msgs::msg::Twist>(
      "cmd_vel", 10,
      std::bind(&DiffDriveNode::cmd_vel_callback, this, std::placeholders::_1));
  }

private:
  void cmd_vel_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    const double V     = msg->linear.x;   // linear velocity  [m/s]
    const double omega = msg->angular.z;  // angular velocity [rad/s]  (BUG FIX: was msg.linear.z)

    const double omega_r = (V + omega * (b_ / 2.0)) / r_;
    const double omega_l = (V - omega * (b_ / 2.0)) / r_;

    std_msgs::msg::Float64 left_msg, right_msg;
    left_msg.data  = omega_l;
    right_msg.data = omega_r;

    left_pub_->publish(left_msg);
    right_pub_->publish(right_msg);
  }

  double b_;  // wheel separation [m]
  double r_;  // wheel radius     [m]

  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr   left_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr   right_pub_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr sub_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DiffDriveNode>());
  rclcpp::shutdown();
  return 0;
}
