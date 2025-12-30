#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>
#include "dolbot/domain/probability_posterior.hpp"
#include "dolbot/domain/probability_prior.hpp"
#include "dolbot/domain/approx_density.hpp"
#include "dolbot/domain/blind_evaluator.hpp"
#include "dolbot/domain/eye_throw.hpp"
#include "dolbot/domain/fortress_ring.hpp"

using namespace dolbot::domain;
using namespace dolbot::core;

int tests_passed = 0;
int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    if (cond) { \
        std::cout << "  PASS: " << msg << std::endl; \
        tests_passed++; \
    } else { \
        std::cout << "  FAIL: " << msg << std::endl; \
        tests_failed++; \
    }

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

int find_chunk_rank(const ChunkList& chunks, int cx, int cz, int tolerance = 2) {
    for (size_t i = 0; i < chunks.size(); ++i) {
        if (std::abs(chunks[i].pos.x - cx) <= tolerance && 
            std::abs(chunks[i].pos.z - cz) <= tolerance) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void test_ring_system() {
    std::cout << "\n=== Ring System Tests ===" << std::endl;
    
    int total = 0;
    for (int i = 0; i < NUM_RINGS; ++i) {
        total += RingSystem::instance().get(i).stronghold_count;
    }
    TEST_ASSERT(total == 128, "Total strongholds = 128");
    
    const auto& ring0 = RingSystem::instance().get(0);
    TEST_ASSERT(ring0.stronghold_count == 3, "First ring has 3 strongholds");
    TEST_ASSERT(ring0.inner_radius > 0, "First ring inner radius > 0");
    TEST_ASSERT(ring0.outer_radius > ring0.inner_radius, "Outer > inner radius");
}

void test_density_distribution() {
    std::cout << "\n=== Density Distribution Tests ===" << std::endl;
    
    auto& density = ApproxDensity::instance();
    
    double total = 0.0;
    for (int r = 0; r < 2000; ++r) {
        total += density.get_density(r) * r * 2.0 * 3.14159265358979;
    }
    TEST_ASSERT(total > 0.5, "Total density > 0.5");
    TEST_ASSERT(total < 2.0, "Total density < 2.0 (normalized)");
    
    const auto& ring0 = RingSystem::instance().get(0);
    double density_in_ring = density.get_density(ring0.center_radius());
    double density_between = density.get_density(ring0.outer_radius + 20);
    TEST_ASSERT(density_in_ring > density_between * 2, 
        "Density higher in rings than between");
    
    double prev = 0.0;
    bool monotonic = true;
    for (int r = 0; r < 2000; r += 10) {
        double curr = density.get_cumulative(r);
        if (curr < prev) monotonic = false;
        prev = curr;
    }
    TEST_ASSERT(monotonic, "Cumulative density is monotonic");
}

void test_triangulation_accuracy() {
    std::cout << "\n=== Triangulation Accuracy Tests ===" << std::endl;
    
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
    
    ProbabilityPosterior posterior(throws, 0.05, 0.001, 0.03, true, ver);
    
    const auto& ranked = posterior.ranked_chunks();
    TEST_ASSERT(!ranked.empty(), "Produces results");
    
    int rank = find_chunk_rank(ranked, target_chunk_x, target_chunk_z);
    TEST_ASSERT(rank >= 0, "Target chunk found in results");
    TEST_ASSERT(rank <= 2, "Target chunk in top 3 (rank=" + std::to_string(rank) + ")");
    
    double certainty = posterior.highest_certainty();
    TEST_ASSERT(certainty > 0.3, "Certainty > 30% for perfect triangulation");
    TEST_ASSERT(certainty <= 1.0, "Certainty <= 100%");
    
    std::cout << "  INFO: Top prediction: chunk (" << ranked[0].pos.x 
              << ", " << ranked[0].pos.z << ") with certainty " 
              << std::fixed << std::setprecision(2) << (certainty * 100) << "%" << std::endl;
}

void test_triangulation_with_error() {
    std::cout << "\n=== Triangulation With Small Errors ===" << std::endl;
    
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
    
    ProbabilityPosterior posterior(throws, 0.05, 0.001, 0.03, true, ver);
    
    const auto& ranked = posterior.ranked_chunks();
    int rank = find_chunk_rank(ranked, target_chunk_x, target_chunk_z, 3);
    TEST_ASSERT(rank >= 0, "Target found with 0.03° error");
    TEST_ASSERT(rank <= 5, "Target in top 5 with small errors");
}

void test_three_throws_better_than_two() {
    std::cout << "\n=== Three Throws vs Two ===" << std::endl;
    
    int target_chunk_x = 85;
    int target_chunk_z = 55;
    McVersion ver = McVersion::V1_19_plus;
    
    std::vector<EyeThrow> two_throws;
    two_throws.push_back(create_throw(0.0, 0.0, 
        angle_to_chunk(0.0, 0.0, target_chunk_x, target_chunk_z, ver)));
    two_throws.push_back(create_throw(500.0, -100.0, 
        angle_to_chunk(500.0, -100.0, target_chunk_x, target_chunk_z, ver)));
    
    ProbabilityPosterior posterior2(two_throws, 0.05, 0.001, 0.03, true, ver);
    double cert2 = posterior2.highest_certainty();
    
    std::vector<EyeThrow> three_throws = two_throws;
    three_throws.push_back(create_throw(-300.0, 300.0, 
        angle_to_chunk(-300.0, 300.0, target_chunk_x, target_chunk_z, ver)));
    
    ProbabilityPosterior posterior3(three_throws, 0.05, 0.001, 0.03, true, ver);
    double cert3 = posterior3.highest_certainty();
    
    TEST_ASSERT(cert3 > cert2, "3 throws better than 2 throws");
    
    std::cout << "  INFO: 2 throws: " << std::fixed << std::setprecision(2) 
              << (cert2 * 100) << "%, 3 throws: " << (cert3 * 100) << "%" << std::endl;
}

void test_probability_normalization() {
    std::cout << "\n=== Probability Normalization ===" << std::endl;
    
    int target_chunk_x = 88;
    int target_chunk_z = 70;
    McVersion ver = McVersion::V1_19_plus;
    
    std::vector<EyeThrow> throws;
    throws.push_back(create_throw(0.0, 0.0, 
        angle_to_chunk(0.0, 0.0, target_chunk_x, target_chunk_z, ver)));
    
    ProbabilityPosterior posterior(throws, 0.05, 0.001, 0.03, true, ver);
    
    double total = 0.0;
    for (const auto& chunk : posterior.ranked_chunks()) {
        total += chunk.probability;
    }
    
    TEST_ASSERT(std::abs(total - 1.0) < 0.05, 
        "Probabilities sum to ~1.0 (actual: " + std::to_string(total) + ")");
}

void test_blind_evaluator() {
    std::cout << "\n=== Blind Evaluator Tests ===" << std::endl;
    
    BlindEvaluator evaluator;
    
    BlindResult result_in_ring = evaluator.evaluate(176.0, 176.0, 400);
    TEST_ASSERT(result_in_ring.evaluation != BlindEvaluation::NotInRing, 
        "Nether (176, 176) is in ring");
    TEST_ASSERT(result_in_ring.highroll_probability > 0.0, 
        "Has highroll probability");
    
    BlindResult result_origin = evaluator.evaluate(5.0, 5.0, 400);
    TEST_ASSERT(result_origin.evaluation == BlindEvaluation::NotInRing, 
        "Near origin is not in ring");
}

void test_variance_model() {
    std::cout << "\n=== Variance Model Tests ===" << std::endl;
    
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
    
    TEST_ASSERT(var_near > var_far, "Variance decreases with distance");
    TEST_ASSERT(var_near > 0, "Near variance > 0");
    TEST_ASSERT(var_far > 0, "Far variance > 0");
    
    std::cout << "  INFO: Variance at 1000 blocks: " << var_near 
              << ", at 10000 blocks: " << var_far << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  DolBot Algorithm Accuracy Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    test_ring_system();
    test_density_distribution();
    test_triangulation_accuracy();
    test_triangulation_with_error();
    test_three_throws_better_than_two();
    test_probability_normalization();
    test_blind_evaluator();
    test_variance_model();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Results: " << tests_passed << " passed, " 
              << tests_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}
