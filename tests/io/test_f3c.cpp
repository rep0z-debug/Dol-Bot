#include <gtest/gtest.h>
#include "dolbot/io/f3c_parser.hpp"

using namespace dolbot::io;
using namespace dolbot::domain;

TEST(F3CParserTest, ParseValidOverworld) {
    std::string input = "/execute in minecraft:overworld run tp @s 100.5 64.0 200.5 45.0 10.0";
    auto result = F3CParser::parse(input);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, 100.5);
    EXPECT_DOUBLE_EQ(result->y, 64.0);
    EXPECT_DOUBLE_EQ(result->z, 200.5);
    EXPECT_DOUBLE_EQ(result->yaw, 45.0);
    EXPECT_DOUBLE_EQ(result->pitch, 10.0);
    EXPECT_EQ(result->dimension, Dimension::Overworld);
}

TEST(F3CParserTest, ParseValidNether) {
    std::string input = "/execute in minecraft:the_nether run tp @s 20.0 50.0 30.0 90.0 0.0";
    auto result = F3CParser::parse(input);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->dimension, Dimension::Nether);
    EXPECT_DOUBLE_EQ(result->x, 20.0);
}

TEST(F3CParserTest, Parse1_12Format) {
    std::string input = "100.5 64.0 200.5 45.0 10.0";
    auto result = F3CParser::parse(input);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, 100.5);
    EXPECT_DOUBLE_EQ(result->dimension, Dimension::Overworld);
}

TEST(F3CParserTest, InvalidInputReturnsNullopt) {
    std::string input = "Not a valid F3+C string";
    auto result = F3CParser::parse(input);
    EXPECT_FALSE(result.has_value());
}

TEST(F3CParserTest, ParseToThrowOverworld) {
    std::string input = "/execute in minecraft:overworld run tp @s 1000.0 64.0 1000.0 180.0 0.0";
    auto result = F3CParser::parse_to_throw(input, 0.0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, ThrowType::Normal);
    EXPECT_FALSE(result->is_boat_mode);
    EXPECT_EQ(result->dimension, Dimension::Overworld);
    EXPECT_DOUBLE_EQ(result->x_overworld(), 1000.0);
}

TEST(F3CParserTest, ParseToThrowNether) {
    std::string input = "/execute in minecraft:the_nether run tp @s 100.0 64.0 100.0 0.0 0.0";
    auto result = F3CParser::parse_to_throw(input, 0.0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->dimension, Dimension::Nether);
    EXPECT_DOUBLE_EQ(result->position.x, 100.0);
    EXPECT_DOUBLE_EQ(result->x_overworld(), 800.0);
}

TEST(F3CParserTest, CrosshairCorrectionApplied) {
    std::string input = "/execute in minecraft:overworld run tp @s 0.0 0.0 0.0 90.0 0.0";
    auto result = F3CParser::parse_to_throw(input, 0.5);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->crosshair_correction, 0.5);
    EXPECT_NEAR(result->corrected_angle(), 90.5, 0.01);
}
