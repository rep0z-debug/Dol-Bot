#include <gtest/gtest.h>
#include "dolbot/domain/fortress_ring.hpp"
#include "dolbot/core/config.hpp"

using namespace dolbot::domain;
using namespace dolbot::core;

TEST(VersionTest, Pre1_19OffsetReturns8) {
    EXPECT_EQ(get_stronghold_chunk_coord(McVersion::Pre_1_9), 8);
    EXPECT_EQ(get_stronghold_chunk_coord(McVersion::V1_9_to_1_12), 8);
    EXPECT_EQ(get_stronghold_chunk_coord(McVersion::V1_13_to_1_18), 8);
}

TEST(VersionTest, Post1_19OffsetReturns0) {
    EXPECT_EQ(get_stronghold_chunk_coord(McVersion::V1_19_plus), 0);
}

TEST(VersionTest, RingSystemStructure) {
    const auto& ring0 = RingSystem::instance().get(0);
    EXPECT_GT(ring0.stronghold_count, 0);
    EXPECT_GT(ring0.outer_radius, ring0.inner_radius);
}
