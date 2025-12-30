#include <gtest/gtest.h>
#include "dolbot/domain/eye_throw.hpp"

using namespace dolbot::domain;

TEST(BoatTest, SnapsToPositiveGrid) {
    EyeThrow t;
    t.is_boat_mode = true;
    t.use_boat_sensitivity = false;
    
    double target = 1.40625;
    t.horizontal_angle = target + 0.1;
    EXPECT_NEAR(t.corrected_angle(), target, 0.001);
    
    t.horizontal_angle = target - 0.1;
    EXPECT_NEAR(t.corrected_angle(), target, 0.001);
}

TEST(BoatTest, SnapsToNegativeGrid) {
    EyeThrow t;
    t.is_boat_mode = true;
    t.use_boat_sensitivity = false;
    
    double target = -0.140625;
    t.horizontal_angle = target - 0.01;
    EXPECT_NEAR(t.corrected_angle(), target, 0.001);
}

TEST(BoatTest, BoatStateValidation) {
    EyeThrow t;
    t.is_boat_mode = true;
    t.boat_error_limit = 0.03;
    
    t.horizontal_angle = 1.40625 + 0.01;
    EXPECT_FALSE(t.is_boat_error());
    
    t.horizontal_angle = 1.40625 + 0.05;
    EXPECT_TRUE(t.is_boat_error());
}

TEST(BoatTest, NormalThrowDoesNotSnap) {
    EyeThrow t;
    t.is_boat_mode = false;
    t.horizontal_angle = 1.5;
    EXPECT_DOUBLE_EQ(t.corrected_angle(), 1.5); // minor packet correction excluded if ~0
}
