#include <gtest/gtest.h>
#include "dolbot/core/coords.hpp"

using namespace dolbot::core;

TEST(CoordsTest, Vec2dLength) {
    Vec2d v{3.0, 4.0};
    EXPECT_DOUBLE_EQ(v.length(), 5.0);
}

TEST(CoordsTest, Vec2dNormalized) {
    Vec2d v{3.0, 4.0};
    auto n = v.normalized();
    EXPECT_NEAR(n.length(), 1.0, 1e-10);
}

TEST(CoordsTest, Vec2dDistance) {
    Vec2d a{0.0, 0.0};
    Vec2d b{3.0, 4.0};
    EXPECT_DOUBLE_EQ(a.distance_to(b), 5.0);
}

TEST(CoordsTest, ChunkPosCenter) {
    ChunkPos c{10, 20};
    EXPECT_EQ(c.center_x(), 168);
    EXPECT_EQ(c.center_z(), 328);
}

TEST(CoordsTest, ChunkPosNether) {
    ChunkPos c{10, 20};
    EXPECT_EQ(c.nether_x(), 20);
    EXPECT_EQ(c.nether_z(), 40);
}

TEST(CoordsTest, AngleNormalization) {
    EXPECT_DOUBLE_EQ(coords::normalize_angle(0.0), 0.0);
    EXPECT_DOUBLE_EQ(coords::normalize_angle(180.0), 180.0);
    EXPECT_DOUBLE_EQ(coords::normalize_angle(-180.0), -180.0);
    EXPECT_NEAR(coords::normalize_angle(270.0), -90.0, 1e-10);
    EXPECT_NEAR(coords::normalize_angle(-270.0), 90.0, 1e-10);
}

TEST(CoordsTest, OverworldNetherConversion) {
    Vec2d ow{800.0, 1600.0};
    auto nether = coords::overworld_to_nether(ow);
    EXPECT_DOUBLE_EQ(nether.x, 100.0);
    EXPECT_DOUBLE_EQ(nether.z, 200.0);
    
    auto back = coords::nether_to_overworld(nether);
    EXPECT_DOUBLE_EQ(back.x, ow.x);
    EXPECT_DOUBLE_EQ(back.z, ow.z);
}
