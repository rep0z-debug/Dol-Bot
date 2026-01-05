#include <gtest/gtest.h>
#include "dolbot/domain/eye_throw.hpp"

using namespace dolbot::domain;
using namespace dolbot::core;

class EyeThrowTest : public ::testing::Test {
protected:
    static constexpr double EPSILON = 0.0001;
};

TEST_F(EyeThrowTest, DefaultConstructor) {
    EyeThrow t;
    
    EXPECT_DOUBLE_EQ(t.position.x, 0.0);
    EXPECT_DOUBLE_EQ(t.position.z, 0.0);
    EXPECT_DOUBLE_EQ(t.horizontal_angle, 0.0);
    EXPECT_EQ(t.type, ThrowType::Normal);
    EXPECT_EQ(t.dimension, Dimension::Overworld);
    EXPECT_FALSE(t.is_boat_mode);
}

TEST_F(EyeThrowTest, ParameterizedConstructor) {
    EyeThrow t(100.0, 200.0, 45.0, 10.0);
    
    EXPECT_DOUBLE_EQ(t.position.x, 100.0);
    EXPECT_DOUBLE_EQ(t.position.z, 200.0);
    EXPECT_DOUBLE_EQ(t.horizontal_angle, 45.0);
    EXPECT_DOUBLE_EQ(t.vertical_angle, 10.0);
}

TEST_F(EyeThrowTest, OverworldPositionFromOverworld) {
    EyeThrow t;
    t.position = {800.0, 1600.0};
    t.dimension = Dimension::Overworld;
    
    auto ow_pos = t.overworld_position();
    EXPECT_DOUBLE_EQ(ow_pos.x, 800.0);
    EXPECT_DOUBLE_EQ(ow_pos.z, 1600.0);
}

TEST_F(EyeThrowTest, OverworldPositionFromNether) {
    EyeThrow t;
    t.position = {100.0, 200.0};
    t.dimension = Dimension::Nether;
    
    auto ow_pos = t.overworld_position();
    EXPECT_DOUBLE_EQ(ow_pos.x, 800.0);   
    EXPECT_DOUBLE_EQ(ow_pos.z, 1600.0);  
}

TEST_F(EyeThrowTest, XOverworldHelper) {
    EyeThrow t;
    t.position = {100.0, 200.0};
    t.dimension = Dimension::Nether;
    
    EXPECT_DOUBLE_EQ(t.x_overworld(), 800.0);
}

TEST_F(EyeThrowTest, ZOverworldHelper) {
    EyeThrow t;
    t.position = {100.0, 200.0};
    t.dimension = Dimension::Nether;
    
    EXPECT_DOUBLE_EQ(t.z_overworld(), 1600.0);
}

TEST_F(EyeThrowTest, CorrectedAngleNormal) {
    EyeThrow t;
    t.horizontal_angle = 45.0;
    t.is_boat_mode = false;
    t.crosshair_correction = 0.5;
    
    double corrected = t.corrected_angle();
    EXPECT_NEAR(corrected, 45.5, 0.1);
}

TEST_F(EyeThrowTest, CorrectedAngleBoatMode) {
    EyeThrow t;
    t.horizontal_angle = 1.5;
    t.is_boat_mode = true;
    t.use_boat_sensitivity = false;
    
    double corrected = t.corrected_angle();
    EXPECT_NEAR(corrected, 1.40625, 0.1);
}

TEST_F(EyeThrowTest, CrosshairCorrectionApplied) {
    EyeThrow t;
    t.horizontal_angle = 90.0;
    t.crosshair_correction = 1.0;
    
    double corrected = t.corrected_angle();
    EXPECT_NEAR(corrected, 91.0, 0.1);
}

TEST_F(EyeThrowTest, SetCrosshairCorrection) {
    EyeThrow t;
    t.set_crosshair_correction(0.75);
    
    EXPECT_DOUBLE_EQ(t.crosshair_correction, 0.75);
}

TEST_F(EyeThrowTest, BoatAngleInfoComputation) {
    EyeThrow t;
    t.is_boat_mode = true;
    t.horizontal_angle = 1.40625;
    
    auto info = t.compute_boat_info();
    
    EXPECT_NEAR(info.snapped_angle, 1.40625, EPSILON);
    EXPECT_NEAR(info.snap_error, 0.0, EPSILON);
}

TEST_F(EyeThrowTest, BoatAngleInfoWithError) {
    EyeThrow t;
    t.is_boat_mode = true;
    t.horizontal_angle = 1.5;
    
    auto info = t.compute_boat_info();
    
    EXPECT_NE(info.snap_error, 0.0);
}

TEST_F(EyeThrowTest, IsBoatErrorFalseWhenValid) {
    EyeThrow t;
    t.is_boat_mode = true;
    t.boat_error_limit = 0.03;
    t.horizontal_angle = 1.40625;
    
    EXPECT_FALSE(t.is_boat_error());
}

TEST_F(EyeThrowTest, IsBoatErrorTrueWhenInvalid) {
    EyeThrow t;
    t.is_boat_mode = true;
    t.boat_error_limit = 0.03;
    t.horizontal_angle = 1.5; 
    
    EXPECT_TRUE(t.is_boat_error());
}

TEST_F(EyeThrowTest, GetBoatState) {
    EyeThrow t;
    t.is_boat_mode = true;
    t.horizontal_angle = 1.40625;
    
    EXPECT_EQ(t.get_boat_state(), BoatState::Valid);
}

TEST_F(EyeThrowTest, BoatStateNoneWhenNotBoatMode) {
    EyeThrow t;
    t.is_boat_mode = false;
    
    EXPECT_EQ(t.get_boat_state(), BoatState::None);
}

TEST_F(EyeThrowTest, AngleRadians) {
    EyeThrow t;
    t.horizontal_angle = 180.0;
    
    double rad = t.angle_radians();
    EXPECT_NEAR(rad, 3.14159265358979, 0.001);
}

TEST_F(EyeThrowTest, DirectionVector) {
    EyeThrow t;
    t.horizontal_angle = 0.0; 
    
    auto dir = t.direction();
    EXPECT_NEAR(dir.x, 0.0, EPSILON);
    EXPECT_LT(dir.z, 0.0);  
}

TEST_F(EyeThrowTest, IsLookingDownTrue) {
    EyeThrow t;
    t.vertical_angle = 45.0;  
    
    EXPECT_TRUE(t.is_looking_down());
}

TEST_F(EyeThrowTest, IsLookingDownFalse) {
    EyeThrow t;
    t.vertical_angle = 10.0;  
    
    EXPECT_FALSE(t.is_looking_down());
}

TEST_F(EyeThrowTest, ApplyCorrection) {
    EyeThrow t;
    t.correction = 0.0;
    
    t.apply_correction(0.5);
    EXPECT_DOUBLE_EQ(t.correction, 0.5);
    
    t.apply_correction(0.3);
    EXPECT_DOUBLE_EQ(t.correction, 0.8);
}

TEST_F(EyeThrowTest, ResetCorrection) {
    EyeThrow t;
    t.correction = 1.5;
    
    t.reset_correction();
    EXPECT_DOUBLE_EQ(t.correction, 0.0);
}

TEST_F(EyeThrowTest, SetSubpixelAdjustment) {
    EyeThrow t;
    t.set_subpixel_adjustment(1080.0);
    
    EXPECT_GT(t.subpixel_adjustment, 0.0);
}

TEST_F(EyeThrowTest, EffectiveTypeNormal) {
    EyeThrow t;
    t.type = ThrowType::Normal;
    t.is_boat_mode = false;
    
    EXPECT_EQ(t.effective_type(), ThrowType::Normal);
}

TEST_F(EyeThrowTest, EffectiveTypeBoat) {
    EyeThrow t;
    t.type = ThrowType::Normal;
    t.is_boat_mode = true;
    
    EXPECT_EQ(t.effective_type(), ThrowType::Boat);
}

TEST_F(EyeThrowTest, GetStandardDeviationNormal) {
    EyeThrow t;
    t.type = ThrowType::Normal;
    t.is_boat_mode = false;
    
    double std = t.get_standard_deviation(0.05, 0.001, 0.03);
    EXPECT_DOUBLE_EQ(std, 0.05);
}

TEST_F(EyeThrowTest, GetStandardDeviationBoat) {
    EyeThrow t;
    t.type = ThrowType::Normal;
    t.is_boat_mode = true;
    
    double std = t.get_standard_deviation(0.05, 0.001, 0.03);
    EXPECT_DOUBLE_EQ(std, 0.001);
}

TEST_F(EyeThrowTest, GetStandardDeviationManual) {
    EyeThrow t;
    t.type = ThrowType::Manual;
    
    double std = t.get_standard_deviation(0.05, 0.001, 0.03);
    EXPECT_DOUBLE_EQ(std, 0.03);
}

TEST_F(EyeThrowTest, FormatAngleNotEmpty) {
    EyeThrow t;
    t.horizontal_angle = 45.123;
    
    EXPECT_FALSE(t.format_angle().empty());
}

TEST_F(EyeThrowTest, FormatCorrectionNotEmpty) {
    EyeThrow t;
    t.correction = 0.5;
    
    EXPECT_FALSE(t.format_correction().empty());
}

TEST_F(EyeThrowTest, BuilderBasicUsage) {
    EyeThrow t = EyeThrowBuilder()
        .at_position(100.0, 200.0)
        .with_angle(45.0)
        .build();
    
    EXPECT_DOUBLE_EQ(t.position.x, 100.0);
    EXPECT_DOUBLE_EQ(t.position.z, 200.0);
    EXPECT_DOUBLE_EQ(t.horizontal_angle, 45.0);
}

TEST_F(EyeThrowTest, BuilderWithVec2d) {
    Vec2d pos{500.0, 600.0};
    EyeThrow t = EyeThrowBuilder()
        .at_position(pos)
        .build();
    
    EXPECT_DOUBLE_EQ(t.position.x, 500.0);
    EXPECT_DOUBLE_EQ(t.position.z, 600.0);
}

TEST_F(EyeThrowTest, BuilderWithBoatMode) {
    EyeThrow t = EyeThrowBuilder()
        .at_position(0.0, 0.0)
        .with_angle(1.40625)
        .in_boat_mode()
        .build();
    
    EXPECT_TRUE(t.is_boat_mode);
}

TEST_F(EyeThrowTest, BuilderWithDimension) {
    EyeThrow t = EyeThrowBuilder()
        .at_position(100.0, 100.0)
        .in_dimension(Dimension::Nether)
        .build();
    
    EXPECT_EQ(t.dimension, Dimension::Nether);
}

TEST_F(EyeThrowTest, BuilderChaining) {
    EyeThrow t = EyeThrowBuilder()
        .at_position(100.0, 200.0)
        .with_angle(45.0)
        .with_vertical_angle(15.0)
        .in_dimension(Dimension::Overworld)
        .with_crosshair_correction(0.5)
        .build();
    
    EXPECT_DOUBLE_EQ(t.horizontal_angle, 45.0);
    EXPECT_DOUBLE_EQ(t.vertical_angle, 15.0);
    EXPECT_DOUBLE_EQ(t.crosshair_correction, 0.5);
}
