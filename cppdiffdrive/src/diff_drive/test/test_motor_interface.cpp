#include <gtest/gtest.h>
#include <cmath>

// ── Replicate just the structs and logic under test ───────────────────────────
// In a real package this would be extracted to a shared header; here we keep
// the test self-contained so it builds without ROS or the Unitree SDK.

static constexpr double GEAR_RATIO = 6.33;
static constexpr double DEFAULT_KW = 0.02;

struct MotorCmdValues {
  int    id;
  int    mode;
  double T;
  double W;
  double Pos;
  double K_P;
  double K_W;
};

// Mirrors MotorInterfaceNode::build_speed_cmd exactly.
MotorCmdValues build_speed_cmd(int motor_id,
                               double omega_output_shaft,
                               double gear_ratio = GEAR_RATIO,
                               double k_w        = DEFAULT_KW)
{
  MotorCmdValues cmd;
  cmd.id   = motor_id;
  cmd.mode = 1;
  cmd.T    = 0.0;
  cmd.Pos  = 0.0;
  cmd.K_P  = 0.0;
  cmd.W    = omega_output_shaft * gear_ratio;
  cmd.K_W  = k_w;
  return cmd;
}

static constexpr double EPS = 1e-9;

// 1. Gear ratio is applied correctly to rotor speed
TEST(BuildSpeedCmd, GearRatioScaling)
{
  // 1 rad/s at output shaft -> 6.33 rad/s at rotor
  auto cmd = build_speed_cmd(0, 1.0);
  EXPECT_NEAR(cmd.W, 1.0 * GEAR_RATIO, EPS);
}

// 2. Negative speed (reverse direction) passes through correctly
TEST(BuildSpeedCmd, NegativeSpeed)
{
  auto cmd = build_speed_cmd(0, -2.0);
  EXPECT_NEAR(cmd.W, -2.0 * GEAR_RATIO, EPS);
}

// 3. Zero speed produces zero rotor speed (damping mode behaviour)
TEST(BuildSpeedCmd, ZeroSpeed)
{
  auto cmd = build_speed_cmd(0, 0.0);
  EXPECT_NEAR(cmd.W, 0.0, EPS);
}

// 4. Speed mode always disables position and feedforward terms
TEST(BuildSpeedCmd, SpeedModeTermsAreZero)
{
  auto cmd = build_speed_cmd(1, 3.0);
  EXPECT_NEAR(cmd.T,   0.0, EPS) << "Feedforward torque must be 0 in speed mode";
  EXPECT_NEAR(cmd.Pos, 0.0, EPS) << "Position target must be 0 in speed mode";
  EXPECT_NEAR(cmd.K_P, 0.0, EPS) << "Position stiffness must be 0 in speed mode";
}

// 5. Motor ID is forwarded unchanged
TEST(BuildSpeedCmd, MotorIdPreserved)
{
  EXPECT_EQ(build_speed_cmd(0, 1.0).id, 0);
  EXPECT_EQ(build_speed_cmd(1, 1.0).id, 1);
}

// 6. FOC mode is always set
TEST(BuildSpeedCmd, ModeIsFOC)
{
  EXPECT_EQ(build_speed_cmd(0, 1.0).mode, 1);
}

// 7. Damping gain is forwarded correctly
TEST(BuildSpeedCmd, DampingGainPreserved)
{
  auto cmd = build_speed_cmd(0, 1.0, GEAR_RATIO, 0.05);
  EXPECT_NEAR(cmd.K_W, 0.05, EPS);
}

// 8. Known end-to-end values: straight line (V=1.0, omega=0)
//    omega_l = omega_r = V/r = 1.0/0.1 = 10 rad/s  =>  rotor = 63.3 rad/s
TEST(BuildSpeedCmd, DiffDriveIntegration_StraightLine)
{
  const double V = 1.0, omega_body = 0.0, b = 0.5, r = 0.1;
  const double omega_l = (V - omega_body * (b / 2.0)) / r;
  const double omega_r = (V + omega_body * (b / 2.0)) / r;

  auto left  = build_speed_cmd(0, omega_l);
  auto right = build_speed_cmd(1, omega_r);

  EXPECT_NEAR(left.W,  10.0 * GEAR_RATIO, EPS);
  EXPECT_NEAR(right.W, 10.0 * GEAR_RATIO, EPS);
}

// 9. Known values: pure rotation (V=0, omega=1.0 rad/s)
//    omega_l = -(1.0 * 0.25) / 0.1 = -2.5 rad/s
//    omega_r =  (1.0 * 0.25) / 0.1 =  2.5 rad/s
TEST(BuildSpeedCmd, DiffDriveIntegration_PureRotation)
{
  const double V = 0.0, omega_body = 1.0, b = 0.5, r = 0.1;
  const double omega_l = (V - omega_body * (b / 2.0)) / r;
  const double omega_r = (V + omega_body * (b / 2.0)) / r;

  auto left  = build_speed_cmd(0, omega_l);
  auto right = build_speed_cmd(1, omega_r);

  EXPECT_NEAR(left.W,  -2.5 * GEAR_RATIO, EPS);
  EXPECT_NEAR(right.W,  2.5 * GEAR_RATIO, EPS);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
