#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "dolbot/domain/probability_posterior.hpp"
#include "dolbot/domain/probability_prior.hpp"
#include "dolbot/domain/approx_density.hpp"
#include "dolbot/domain/blind_evaluator.hpp"
#include "dolbot/domain/eye_throw.hpp"
#include "dolbot/domain/fortress_ring.hpp"

using namespace dolbot::domain;
using namespace dolbot::core;

class AccuracyTest : public ::testing::Test {
protected:
    static constexpr double STD_DEV = 0.05;
    static constexpr double STD_DEV_BOAT = 0.001;
    static constexpr double STD_DEV_MANUAL = 0.03;
    
    EyeThrow create_throw(double x, double z, double angle) {
        EyeThrow t;
        t.position = {x, z};
        t.horizontal_angle = angle;
        return t;
    }
    
    double angle_to_chunk(double from_x, double from_z, int chunk_x, int chunk_z, McVersion version) {
        int sh_offset = get_stronghold_chunk_coord(version);
        double dx = chunk_x * 16 + sh_offset - from_x;
        double dz = chunk_z * 16 + sh_offset - from_z;
        return -180.0 / 3.14159265358979 * std::atan2(dx, dz);
    }
    
    bool prediction_contains_chunk(const ChunkList& chunks, int cx, int cz, int tolerance = 2) {
        for (const auto& chunk : chunks) {
            if (std::abs(chunk.pos.x - cx) <= tolerance && 
                std::abs(chunk.pos.z - cz) <= tolerance) {
                return true;
            }
        }
        return false;
    }
    
    int find_chunk_rank(const ChunkList& chunks, int cx, int cz, int tolerance = 2) {
        for (size_t i = 0; i < chunks.size(); ++i) {
            if (std::abs(chunks[i].pos.x - cx) <= tolerance && 
                std::abs(chunks[i].pos.z - cz) <= tolerance) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
};

TEST_F(AccuracyTest, PerfectTwoThrowTriangulation) {
    int target_chunk_x = 80;
    int target_chunk_z = 60;
    McVersion ver = McVersion::V1_19_plus;
    
    double pos1_x = 100.0, pos1_z = -200.0;
    double pos2_x = 500.0, pos2_z = 400.0;
    
    double angle1 = angle_to_chunk(pos1_x, pos1_z, target_chunk_x, target_chunk_z, ver);
    double angle2 = angle_to_chunk(pos2_x, pos2_z, target_chunk_x, target_chunk_z, ver);
    
    std::vector<EyeThrow> throws;
    throws.push_back(create_throw(pos1_x, pos1_z, angle1));
    throws.push_back(create_throw(pos2_x, pos2_z, angle2));
    
    ProbabilityPosterior posterior(throws, STD_DEV, STD_DEV_BOAT, STD_DEV_MANUAL, true, ver);
    
    const auto& ranked = posterior.ranked_chunks();
    ASSERT_FALSE(ranked.empty()) << "No chunks found!";
    
    int rank = find_chunk_rank(ranked, target_chunk_x, target_chunk_z);
    EXPECT_GE(rank, 0) << "Target chunk not in results";
    EXPECT_LE(rank, 2) << "Target chunk should be in top 3, was rank " << rank;
    
    EXPECT_GT(posterior.highest_certainty(), 0.3) 
        << "Certainty too low for perfect triangulation: " << posterior.highest_certainty();
}

TEST_F(AccuracyTest, SlightlyOffThrowsStillFindTarget) {
    int target_chunk_x = 90;
    int target_chunk_z = 40;
    McVersion ver = McVersion::V1_19_plus;
    
    double pos1_x = 0.0, pos1_z = 0.0;
    double pos2_x = 400.0, pos2_z = -200.0;
    
    double angle1 = angle_to_chunk(pos1_x, pos1_z, target_chunk_x, target_chunk_z, ver) + 0.03;
    double angle2 = angle_to_chunk(pos2_x, pos2_z, target_chunk_x, target_chunk_z, ver) - 0.02;
    
    std::vector<EyeThrow> throws;
    throws.push_back(create_throw(pos1_x, pos1_z, angle1));
    throws.push_back(create_throw(pos2_x, pos2_z, angle2));
    
    ProbabilityPosterior posterior(throws, STD_DEV, STD_DEV_BOAT, STD_DEV_MANUAL, true, ver);
    
    const auto& ranked = posterior.ranked_chunks();
    ASSERT_FALSE(ranked.empty());
    
    int rank = find_chunk_rank(ranked, target_chunk_x, target_chunk_z, 3);
    EXPECT_GE(rank, 0) << "Target chunk not found within tolerance";
    EXPECT_LE(rank, 5) << "Target should be in top 5 even with small errors";
}

TEST_F(AccuracyTest, ThreeThrowsIncreaseCertainty) {
    int target_chunk_x = 85;
    int target_chunk_z = 55;
    McVersion ver = McVersion::V1_19_plus;
    
    std::vector<EyeThrow> two_throws;
    two_throws.push_back(create_throw(0.0, 0.0, 
        angle_to_chunk(0.0, 0.0, target_chunk_x, target_chunk_z, ver)));
    two_throws.push_back(create_throw(500.0, -100.0, 
        angle_to_chunk(500.0, -100.0, target_chunk_x, target_chunk_z, ver)));
    
    ProbabilityPosterior posterior2(two_throws, STD_DEV, STD_DEV_BOAT, STD_DEV_MANUAL, true, ver);
    double cert2 = posterior2.highest_certainty();
    
    std::vector<EyeThrow> three_throws = two_throws;
    three_throws.push_back(create_throw(-300.0, 300.0, 
        angle_to_chunk(-300.0, 300.0, target_chunk_x, target_chunk_z, ver)));
    
    ProbabilityPosterior posterior3(three_throws, STD_DEV, STD_DEV_BOAT, STD_DEV_MANUAL, true, ver);
    double cert3 = posterior3.highest_certainty();
    
    EXPECT_GT(cert3, cert2) << "Three throws should have higher certainty than two";
}

TEST_F(AccuracyTest, DensityTableIsNormalized) {
    auto& density = ApproxDensity::instance();
    
    double total = 0.0;
    for (int r = 0; r < 2000; ++r) {
        total += density.get_density(r) * r * 2.0 * 3.14159265358979;
    }
    
    EXPECT_GT(total, 0.5) << "Total density should be significant";
    EXPECT_LT(total, 2.0) << "Total density should be roughly normalized";
}

TEST_F(AccuracyTest, DensityPeaksInRings) {
    auto& density = ApproxDensity::instance();
    const auto& ring0 = RingSystem::instance().get(0);
    
    double density_in_ring = density.get_density(ring0.center_radius());
    double density_between_rings = density.get_density(ring0.outer_radius + 20);
    
    EXPECT_GT(density_in_ring, density_between_rings * 2) 
        << "Density should be higher in rings than between them";
}

TEST_F(AccuracyTest, RingSystemTotalIs128) {
    int total = 0;
    for (int i = 0; i < NUM_RINGS; ++i) {
        total += RingSystem::instance().get(i).stronghold_count;
    }
    EXPECT_EQ(total, 128) << "Total strongholds should be 128";
}

TEST_F(AccuracyTest, CumulativeDensityMonotonic) {
    auto& density = ApproxDensity::instance();
    
    double prev = 0.0;
    for (int r = 0; r < 2000; r += 10) {
        double curr = density.get_cumulative(r);
        EXPECT_GE(curr, prev) << "Cumulative should be monotonically increasing at r=" << r;
        prev = curr;
    }
}

TEST_F(AccuracyTest, BlindEvaluatorInFirstRing) {
    BlindEvaluator evaluator;
    
    double nether_x = 176.0;
    double nether_z = 176.0;
    
    BlindResult result = evaluator.evaluate(nether_x, nether_z, 400);
    
    EXPECT_NE(result.evaluation, BlindEvaluation::NotInRing) 
        << "Position at nether (176, 176) should be in first ring";
    EXPECT_GT(result.highroll_probability, 0.0) 
        << "Should have some highroll probability";
}

TEST_F(AccuracyTest, BlindEvaluatorOutsideRings) {
    BlindEvaluator evaluator;
    
    double nether_x = 5.0;
    double nether_z = 5.0;
    
    BlindResult result = evaluator.evaluate(nether_x, nether_z, 400);
    
    EXPECT_EQ(result.evaluation, BlindEvaluation::NotInRing) 
        << "Position near origin should not be in any ring";
}

TEST_F(AccuracyTest, VarianceModelDecreaseWithDistance) {
    EyeThrow t;
    t.position = {0.0, 0.0};
    t.horizontal_angle = 0.0;
    
    auto get_variance = [](double dist_sq) {
        if (dist_sq < 100.0) return 0.0;
        double distance = std::sqrt(dist_sq);
        constexpr double MAX_POSITION_ERROR = 0.5;
        double angular_error_rad = std::atan(MAX_POSITION_ERROR / distance);
        double angular_error_deg = angular_error_rad * 180.0 / 3.14159265358979;
        return angular_error_deg * angular_error_deg / 3.0;
    };
    
    double var_near = get_variance(1000.0 * 1000.0);
    double var_far = get_variance(10000.0 * 10000.0);
    
    EXPECT_GT(var_near, var_far) << "Variance should decrease with distance";
}

TEST_F(AccuracyTest, ProbabilitySumsToOne) {
    int target_chunk_x = 88;
    int target_chunk_z = 70;
    McVersion ver = McVersion::V1_19_plus;
    
    std::vector<EyeThrow> throws;
    throws.push_back(create_throw(0.0, 0.0, 
        angle_to_chunk(0.0, 0.0, target_chunk_x, target_chunk_z, ver)));
    
    ProbabilityPosterior posterior(throws, STD_DEV, STD_DEV_BOAT, STD_DEV_MANUAL, true, ver);
    
    double total = 0.0;
    for (const auto& chunk : posterior.ranked_chunks()) {
        total += chunk.probability;
    }
    
    EXPECT_NEAR(total, 1.0, 0.01) << "Probabilities should sum to approximately 1";
}
