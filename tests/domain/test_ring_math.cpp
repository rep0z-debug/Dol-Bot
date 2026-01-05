#include <gtest/gtest.h>
#include "dolbot/domain/fortress_ring.hpp"

using namespace dolbot::domain;

TEST(RingMathTest, RingSystemHasEightRings) {
    const auto& rings = RingSystem::instance().all();
    EXPECT_EQ(rings.size(), 8);
}

TEST(RingMathTest, RingZeroHasThreeStrongholds) {
    const auto& ring = RingSystem::instance().get(0);
    EXPECT_EQ(ring.stronghold_count, 3);
}

TEST(RingMathTest, TotalStrongholds128) {
    const auto& rings = RingSystem::instance().all();
    int total = 0;
    for (const auto& r : rings) {
        total += r.stronghold_count;
    }
    EXPECT_EQ(total, TOTAL_STRONGHOLDS);
}

TEST(RingMathTest, RingRadiiIncreasing) {
    const auto& rings = RingSystem::instance().all();
    for (size_t i = 1; i < rings.size(); ++i) {
        EXPECT_GT(rings[i].inner_radius, rings[i-1].outer_radius);
    }
}

TEST(RingMathTest, FindRingReturnsCorrect) {
    const auto* ring = RingSystem::instance().find_ring(100.0);
    EXPECT_NE(ring, nullptr);
    EXPECT_TRUE(ring->contains_radius(100.0));
}

TEST(RingMathTest, FindClosestRingsReturnsTwo) {
    auto [r1, r2] = RingSystem::instance().find_closest_rings(50.0, 50.0);
    EXPECT_NE(r1, nullptr);
    EXPECT_NE(r2, nullptr);
    EXPECT_NE(r1, r2);
}

TEST(RingMathTest, CenterRadiusIsMiddle) {
    const auto& ring = RingSystem::instance().get(0);
    double expected = (ring.inner_radius + ring.outer_radius) / 2.0;
    EXPECT_DOUBLE_EQ(ring.center_radius(), expected);
}
