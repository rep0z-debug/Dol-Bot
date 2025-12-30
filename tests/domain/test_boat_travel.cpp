#include <gtest/gtest.h>
#include "dolbot/domain/boat_travel.hpp"

using namespace dolbot::domain;

class BoatTravelTest : public ::testing::Test {
protected:
    static constexpr double EPSILON = 0.0001;
};

TEST_F(BoatTravelTest, BoatIncrementValue) {
    EXPECT_DOUBLE_EQ(BoatTravel::BOAT_INCREMENT, 1.40625);
}

TEST_F(BoatTravelTest, BoatIncrementSmallValue) {
    EXPECT_DOUBLE_EQ(BoatTravel::BOAT_INCREMENT_SMALL, 0.140625);
}

TEST_F(BoatTravelTest, SnapAnglePositive) {
    auto snapped = BoatTravel::snap_angle(1.5);
    ASSERT_TRUE(snapped.has_value());
    EXPECT_NEAR(*snapped, 1.40625, EPSILON);
}

TEST_F(BoatTravelTest, SnapAngleNegative) {
    auto snapped = BoatTravel::snap_angle(-0.1);
    ASSERT_TRUE(snapped.has_value());
    EXPECT_NEAR(*snapped, -0.140625, EPSILON);
}

TEST_F(BoatTravelTest, SnapAngleExact) {
    auto snapped = BoatTravel::snap_angle(1.40625);
    ASSERT_TRUE(snapped.has_value());
    EXPECT_DOUBLE_EQ(*snapped, 1.40625);
}

TEST_F(BoatTravelTest, SnapAngleOutOfRange) {
    auto snapped = BoatTravel::snap_angle(400.0);
    EXPECT_FALSE(snapped.has_value());
}

TEST_F(BoatTravelTest, SnapAngleZero) {
    auto snapped = BoatTravel::snap_angle(0.0);
    ASSERT_TRUE(snapped.has_value());
    EXPECT_DOUBLE_EQ(*snapped, 0.0);
}

TEST_F(BoatTravelTest, IsValidBoatAngleExact) {
    EXPECT_TRUE(BoatTravel::is_valid_boat_angle(1.40625, 0.03));
}

TEST_F(BoatTravelTest, IsValidBoatAngleWithinTolerance) {
    EXPECT_TRUE(BoatTravel::is_valid_boat_angle(1.42, 0.03));
}

TEST_F(BoatTravelTest, IsValidBoatAngleOutsideTolerance) {
    EXPECT_FALSE(BoatTravel::is_valid_boat_angle(1.50, 0.03));
}

TEST_F(BoatTravelTest, IsValidBoatAngleNegativeExact) {
    EXPECT_TRUE(BoatTravel::is_valid_boat_angle(-0.140625, 0.03));
}

TEST_F(BoatTravelTest, IsValidBoatAngleOutOfRange) {
    EXPECT_FALSE(BoatTravel::is_valid_boat_angle(400.0, 0.03));
}

TEST_F(BoatTravelTest, MinAngleIncrementLowSensitivity) {
    double inc_low = BoatTravel::get_min_angle_increment(0.0);
    double inc_high = BoatTravel::get_min_angle_increment(1.0);
    
    EXPECT_GT(inc_high, inc_low);
}

TEST_F(BoatTravelTest, MinAngleIncrementPositive) {
    for (double sens = 0.0; sens <= 1.0; sens += 0.1) {
        double inc = BoatTravel::get_min_angle_increment(sens);
        EXPECT_GT(inc, 0.0) << "Sensitivity: " << sens;
    }
}

TEST_F(BoatTravelTest, PreciseBoatAngle) {
    double alpha = 45.5;
    double sensitivity = 0.5;
    double boat_angle = 45.0;
    
    double precise = BoatTravel::get_precise_boat_angle(alpha, sensitivity, boat_angle);
    
    EXPECT_NE(precise, alpha);
}

TEST_F(BoatTravelTest, IsValidBoatAngleWithSensitivity) {
    double sensitivity = 0.5;
    double min_inc = BoatTravel::get_min_angle_increment(sensitivity);
    double valid_angle = std::round(45.0 / min_inc) * min_inc;
    
    EXPECT_TRUE(BoatTravel::is_valid_boat_angle_with_sensitivity(valid_angle, sensitivity, 0.03));
}

TEST_F(BoatTravelTest, SnapAngleWithSensitivity) {
    auto snapped = BoatTravel::snap_angle_with_sensitivity(45.5, 0.5, 45.0);
    ASSERT_TRUE(snapped.has_value());
}

TEST_F(BoatTravelTest, SnapErrorZeroForExact) {
    double error = BoatTravel::get_snap_error(1.40625);
    EXPECT_NEAR(error, 0.0, EPSILON);
}

TEST_F(BoatTravelTest, SnapErrorNonZeroForOff) {
    double error = BoatTravel::get_snap_error(1.5);
    EXPECT_NE(error, 0.0);
}

TEST_F(BoatTravelTest, SnapErrorSign) {
    double error_positive = BoatTravel::get_snap_error(1.5); 
    EXPECT_GT(error_positive, 0.0);
    
    double error_negative = BoatTravel::get_snap_error(1.3);  
    EXPECT_LT(error_negative, 0.0);
}

TEST_F(BoatTravelTest, GridIndexPositive) {
    int index = BoatTravel::get_grid_index(2.8125);
    EXPECT_EQ(index, 2);
}

TEST_F(BoatTravelTest, GridIndexNegative) {
    int index = BoatTravel::get_grid_index(-0.281250);
    EXPECT_EQ(index, -2);
}

TEST_F(BoatTravelTest, GridIndexZero) {
    int index = BoatTravel::get_grid_index(0.0);
    EXPECT_EQ(index, 0);
}

TEST_F(BoatTravelTest, ReduceAngleMod360) {
    double current_boat = 400.0;
    double player = 410.0;
    double sensitivity = 0.5;
    
    double reduced = BoatTravel::reduce_angle_mod360(current_boat, player, sensitivity);
    
    EXPECT_LT(std::abs(reduced), std::abs(current_boat));
}

TEST_F(BoatTravelTest, ComputeBoatStateValid) {
    auto state = BoatTravel::compute_boat_state(1.40625, 0.03);
    
    EXPECT_TRUE(state.is_valid);
    EXPECT_NEAR(state.angle, 1.40625, EPSILON);
    EXPECT_NEAR(state.snap_error, 0.0, EPSILON);
}

TEST_F(BoatTravelTest, ComputeBoatStateInvalid) {
    auto state = BoatTravel::compute_boat_state(1.5, 0.03);
    
    EXPECT_FALSE(state.is_valid);
    EXPECT_NE(state.snap_error, 0.0);
}

TEST_F(BoatTravelTest, ComputeBoatStateGridIndex) {
    auto state = BoatTravel::compute_boat_state(2.8125, 0.03);
    
    EXPECT_EQ(state.grid_index, 2);
}

TEST_F(BoatTravelTest, ComputeBoatStateWithSensitivity) {
    auto state = BoatTravel::compute_boat_state_with_sensitivity(
        45.0, 0.5, 44.0, 0.03
    );
    
    EXPECT_FALSE(state.is_entering);
    EXPECT_GE(state.grid_index, 0);
}

TEST_F(BoatTravelTest, ComputeBoatStateWithSensitivityValid) {
    double sensitivity = 0.5;
    double min_inc = BoatTravel::get_min_angle_increment(sensitivity);
    double valid_angle = std::round(45.0 / min_inc) * min_inc;
    
    auto state = BoatTravel::compute_boat_state_with_sensitivity(
        valid_angle, sensitivity, valid_angle, 0.03
    );
    
    EXPECT_TRUE(state.is_valid);
}

TEST_F(BoatTravelTest, HandleZeroSensitivity) {
    double inc = BoatTravel::get_min_angle_increment(0.0);
    EXPECT_GT(inc, 0.0);
}

TEST_F(BoatTravelTest, HandleMaxSensitivity) {
    double inc = BoatTravel::get_min_angle_increment(1.0);
    EXPECT_GT(inc, 0.0);
}

TEST_F(BoatTravelTest, SnapAtBoundary180) {
    auto snapped = BoatTravel::snap_angle(180.0);
    ASSERT_TRUE(snapped.has_value());
}

TEST_F(BoatTravelTest, SnapAtBoundaryNeg180) {
    auto snapped = BoatTravel::snap_angle(-180.0);
    ASSERT_TRUE(snapped.has_value());
}
