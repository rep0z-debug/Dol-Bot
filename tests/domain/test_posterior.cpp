#include <gtest/gtest.h>
#include "dolbot/domain/probability_posterior.hpp"
#include "dolbot/domain/eye_throw.hpp"

using namespace dolbot::domain;
using namespace dolbot::core;

TEST(PosteriorTest, TwoThrowsProducesValidResult) {
    std::vector<EyeThrow> throws;
    
    EyeThrow t1;
    t1.position = {200.0, 100.0};
    t1.horizontal_angle = -45.0;
    throws.push_back(t1);
    
    EyeThrow t2;
    t2.position = {-300.0, 200.0};
    t2.horizontal_angle = 30.0;
    throws.push_back(t2);
    
    ProbabilityPosterior posterior(throws, 0.05, true, McVersion::V1_13_to_1_18);
    
    EXPECT_FALSE(posterior.ranked_chunks().empty());
    EXPECT_GT(posterior.highest_certainty(), 0.0);
    EXPECT_LE(posterior.highest_certainty(), 1.0);
}

TEST(PosteriorTest, ParallelThrowsLowCertainty) {
    std::vector<EyeThrow> throws;
    
    EyeThrow t1;
    t1.position = {0.0, 0.0};
    t1.horizontal_angle = 0.0;
    throws.push_back(t1);
    
    EyeThrow t2;
    t2.position = {100.0, 0.0};
    t2.horizontal_angle = 0.0;
    throws.push_back(t2);
    
    ProbabilityPosterior posterior(throws, 0.05, true, McVersion::V1_13_to_1_18);
    
    EXPECT_LT(posterior.highest_certainty(), 0.5);
}

TEST(PosteriorTest, TopPredictionsReturnsCorrectCount) {
    std::vector<EyeThrow> throws;
    
    EyeThrow t1;
    t1.position = {500.0, 300.0};
    t1.horizontal_angle = -60.0;
    throws.push_back(t1);
    
    EyeThrow t2;
    t2.position = {-200.0, 500.0};
    t2.horizontal_angle = 45.0;
    throws.push_back(t2);
    
    ProbabilityPosterior posterior(throws, 0.05, true, McVersion::V1_13_to_1_18);
    
    auto top3 = posterior.top_predictions(3);
    EXPECT_LE(top3.size(), 3);
    
    for (const auto& pred : top3) {
        EXPECT_GE(pred.certainty, 0.0);
        EXPECT_LE(pred.certainty, 1.0);
    }
}
