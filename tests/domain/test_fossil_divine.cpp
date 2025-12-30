#include <gtest/gtest.h>
#include "dolbot/domain/fossil_divine.hpp"

using namespace dolbot::domain;
using namespace dolbot::core;

class FossilDivineTest : public ::testing::Test {
protected:
    static constexpr double EPSILON = 0.001;
};

TEST_F(FossilDivineTest, ParseValidBoneBlockXAxis) {
    std::string input = "/setblock 100 64 200 minecraft:bone_block[axis=x]";
    auto result = FossilDivine::parse_f3i_bone(input);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, 100.0);
    EXPECT_DOUBLE_EQ(result->z, 200.0);
    EXPECT_GE(result->segment, 0);
    EXPECT_LT(result->segment, 16);
}

TEST_F(FossilDivineTest, ParseValidBoneBlockZAxis) {
    std::string input = "/setblock -500 32 750 minecraft:bone_block[axis=z]";
    auto result = FossilDivine::parse_f3i_bone(input);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, -500.0);
    EXPECT_DOUBLE_EQ(result->z, 750.0);
}

TEST_F(FossilDivineTest, ParseInvalidBoneBlock) {
    std::string input = "Not a bone block command";
    auto result = FossilDivine::parse_f3i_bone(input);
    EXPECT_FALSE(result.has_value());
}

TEST_F(FossilDivineTest, ParseEmptyString) {
    std::string input = "";
    auto result = FossilDivine::parse_f3i_bone(input);
    EXPECT_FALSE(result.has_value());
}

TEST_F(FossilDivineTest, ParseWrongBlockType) {
    std::string input = "/setblock 100 64 200 minecraft:stone";
    auto result = FossilDivine::parse_f3i_bone(input);
    EXPECT_FALSE(result.has_value());
}

TEST_F(FossilDivineTest, SegmentFromPositionNorth) {
    int segment = FossilDivine::segment_from_position(0.0, -100.0);
    EXPECT_GE(segment, 0);
    EXPECT_LT(segment, FossilDivine::SEGMENTS);
}

TEST_F(FossilDivineTest, SegmentFromPositionSouth) {
    int segment = FossilDivine::segment_from_position(0.0, 100.0);
    EXPECT_GE(segment, 0);
    EXPECT_LT(segment, FossilDivine::SEGMENTS);
}

TEST_F(FossilDivineTest, SegmentFromPositionEast) {
    int segment = FossilDivine::segment_from_position(100.0, 0.0);
    EXPECT_GE(segment, 0);
    EXPECT_LT(segment, FossilDivine::SEGMENTS);
}

TEST_F(FossilDivineTest, SegmentFromPositionWest) {
    int segment = FossilDivine::segment_from_position(-100.0, 0.0);
    EXPECT_GE(segment, 0);
    EXPECT_LT(segment, FossilDivine::SEGMENTS);
}

TEST_F(FossilDivineTest, SegmentAlwaysInValidRange) {
    for (int i = 0; i < 360; i += 15) {
        double rad = i * 3.14159265358979 / 180.0;
        double x = std::cos(rad) * 1000.0;
        double z = std::sin(rad) * 1000.0;
        int segment = FossilDivine::segment_from_position(x, z);
        EXPECT_GE(segment, 0) << "Angle: " << i;
        EXPECT_LT(segment, FossilDivine::SEGMENTS) << "Angle: " << i;
    }
}

TEST_F(FossilDivineTest, ComputeReturnsValidResult) {
    FossilLocation fossil{100.0, 200.0, 5};
    auto result = FossilDivine::compute(fossil);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_valid());
}

TEST_F(FossilDivineTest, ComputeDirectionAngleInRange) {
    FossilLocation fossil{500.0, 500.0, 3};
    auto result = FossilDivine::compute(fossil);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result->direction_angle, -180.0);
    EXPECT_LE(result->direction_angle, 180.0);
}

TEST_F(FossilDivineTest, ComputeOppositeAngle180Apart) {
    FossilLocation fossil{1000.0, 0.0, 4};
    auto result = FossilDivine::compute(fossil);
    
    ASSERT_TRUE(result.has_value());
    double diff = std::abs(result->direction_angle - result->opposite_angle);
    EXPECT_NEAR(std::min(diff, 360.0 - diff), 180.0, 1.0);
}

TEST_F(FossilDivineTest, ComputeInvalidSegment) {
    FossilLocation fossil{100.0, 100.0, -1};
    auto result = FossilDivine::compute(fossil);
    EXPECT_FALSE(result.has_value());
    
    FossilLocation fossil2{100.0, 100.0, 16};
    auto result2 = FossilDivine::compute(fossil2);
    EXPECT_FALSE(result2.has_value());
}

TEST_F(FossilDivineTest, ComputeRingIndexValid) {
    FossilLocation fossil{1000.0, 1000.0, 2};
    auto result = FossilDivine::compute(fossil);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result->ring_index, 0);
    EXPECT_LE(result->ring_index, 7);
}

TEST_F(FossilDivineTest, SegmentAngleRangeValid) {
    for (int segment = 0; segment < FossilDivine::SEGMENTS; ++segment) {
        auto range = FossilDivine::get_segment_angle_range(segment);
        EXPECT_LT(range[0], range[1]) << "Segment: " << segment;
        EXPECT_GE(range[0], 0.0) << "Segment: " << segment;
        EXPECT_LE(range[1], 360.0) << "Segment: " << segment;
    }
}

TEST_F(FossilDivineTest, SegmentAngleRangeInvalidSegment) {
    auto range = FossilDivine::get_segment_angle_range(-1);
    EXPECT_DOUBLE_EQ(range[0], 0.0);
    EXPECT_DOUBLE_EQ(range[1], 0.0);
    
    auto range2 = FossilDivine::get_segment_angle_range(16);
    EXPECT_DOUBLE_EQ(range2[0], 0.0);
    EXPECT_DOUBLE_EQ(range2[1], 0.0);
}

TEST_F(FossilDivineTest, SegmentsCoverFullCircle) {
    double total_coverage = 0.0;
    for (int segment = 0; segment < FossilDivine::SEGMENTS; ++segment) {
        auto range = FossilDivine::get_segment_angle_range(segment);
        total_coverage += (range[1] - range[0]);
    }
    EXPECT_NEAR(total_coverage, 360.0, 0.01);
}

TEST_F(FossilDivineTest, DivineCoordsAtDistance) {
    FossilLocation fossil{500.0, 500.0, 4};
    double target_distance = 1000.0;
    
    Vec2d coords = FossilDivine::get_divine_coords(fossil, target_distance);
    double actual_distance = coords.length();
    
    EXPECT_NEAR(actual_distance, target_distance, 1.0);
}

TEST_F(FossilDivineTest, DivineCoordsZeroDistance) {
    FossilLocation fossil{100.0, 100.0, 0};
    Vec2d coords = FossilDivine::get_divine_coords(fossil, 0.0);
    
    EXPECT_NEAR(coords.x, 0.0, EPSILON);
    EXPECT_NEAR(coords.z, 0.0, EPSILON);
}
