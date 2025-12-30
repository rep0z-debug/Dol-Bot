#include <gtest/gtest.h>
#include "dolbot/domain/throw_advisor.hpp"
#include "dolbot/domain/eye_throw.hpp"
#include "dolbot/domain/probability_posterior.hpp"

using namespace dolbot::domain;
using namespace dolbot::core;

class ThrowAdvisorTest : public ::testing::Test {
protected:
    ThrowAdvisor advisor;
    
    WorldChunk create_chunk(int x, int z, double prob) {
        WorldChunk chunk;
        chunk.pos = {x, z};
        chunk.probability = prob;
        return chunk;
    }
    
    EyeThrow create_throw(double x, double z, double angle) {
        EyeThrow t;
        t.position = {x, z};
        t.horizontal_angle = angle;
        return t;
    }
};

TEST_F(ThrowAdvisorTest, AnalyzeWithEmptyPredictions) {
    ChunkList empty;
    Vec2d pos{0.0, 0.0};
    
    ThrowAdvice advice = advisor.analyze(empty, pos, 0.0, 0.05);
    
    EXPECT_FALSE(advice.should_throw);
}

TEST_F(ThrowAdvisorTest, AnalyzeWithValidPredictions) {
    ChunkList chunks;
    chunks.push_back(create_chunk(80, 60, 0.5));
    chunks.push_back(create_chunk(82, 58, 0.3));
    chunks.push_back(create_chunk(78, 62, 0.2));
    
    Vec2d pos{100.0, 100.0};
    
    ThrowAdvice advice = advisor.analyze(chunks, pos, 0.5, 0.05);
    
    EXPECT_TRUE(advice.should_throw);
    EXPECT_GT(advice.expected_certainty_gain, 0.0);
}

TEST_F(ThrowAdvisorTest, AnalyzeHighCertaintyNoThrowNeeded) {
    ChunkList chunks;
    chunks.push_back(create_chunk(80, 60, 0.95));
    
    Vec2d pos{100.0, 100.0};
    
    ThrowAdvice advice = advisor.analyze(chunks, pos, 0.95, 0.05);
    
    EXPECT_FALSE(advice.should_throw);
}

TEST_F(ThrowAdvisorTest, AnalyzeSecondThrowPerpendicular) {
    EyeThrow first = create_throw(0.0, 0.0, 45.0);
    WorldChunk predicted = create_chunk(80, 80, 0.6);
    
    SecondThrowAdvice advice = advisor.analyze_second_throw(
        first, predicted, McVersion::V1_19_plus
    );
    
    EXPECT_GT(advice.distance_to_travel, 0.0);
    EXPECT_GE(advice.travel_angle, -180.0);
    EXPECT_LE(advice.travel_angle, 180.0);
}

TEST_F(ThrowAdvisorTest, AnalyzeSecondThrowQualityRating) {
    EyeThrow first = create_throw(0.0, 0.0, 30.0);
    WorldChunk predicted = create_chunk(90, 45, 0.5);
    
    SecondThrowAdvice advice = advisor.analyze_second_throw(
        first, predicted, McVersion::V1_13_to_1_18
    );
    
    EXPECT_FALSE(advice.quality_rating.empty());
}

TEST_F(ThrowAdvisorTest, ComputeOptimalPositionWithThrows) {
    std::vector<EyeThrow> throws;
    throws.push_back(create_throw(0.0, 0.0, 45.0));
    
    std::vector<PredictionResult> predictions;
    PredictionResult pred;
    pred.chunk_x = 80;
    pred.chunk_z = 60;
    pred.certainty = 0.5;
    predictions.push_back(pred);
    
    auto result = advisor.compute_optimal_second_position(
        throws, predictions, McVersion::V1_19_plus
    );
    
    if (result.has_value()) {
        double dist = result->length();
        EXPECT_GT(dist, 0.0);
    }
}

TEST_F(ThrowAdvisorTest, ComputeOptimalPositionEmptyThrows) {
    std::vector<EyeThrow> throws;
    std::vector<PredictionResult> predictions;
    
    auto result = advisor.compute_optimal_second_position(
        throws, predictions, McVersion::V1_19_plus
    );
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(ThrowAdvisorTest, ComputeOptimalPositionHighCertainty) {
    std::vector<EyeThrow> throws;
    throws.push_back(create_throw(0.0, 0.0, 45.0));
    
    std::vector<PredictionResult> predictions;
    PredictionResult pred;
    pred.chunk_x = 80;
    pred.chunk_z = 60;
    pred.certainty = 0.95;  
    predictions.push_back(pred);
    
    auto result = advisor.compute_optimal_second_position(
        throws, predictions, McVersion::V1_19_plus
    );
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(ThrowAdvisorTest, DirectionDescriptionNotEmpty) {
    ChunkList chunks;
    chunks.push_back(create_chunk(80, 60, 0.5));
    
    Vec2d pos{0.0, 0.0};
    
    ThrowAdvice advice = advisor.analyze(chunks, pos, 0.5, 0.05);
    
    if (advice.should_throw) {
        EXPECT_FALSE(advice.direction_description.empty());
    }
}

TEST_F(ThrowAdvisorTest, CertaintyGainNonNegative) {
    ChunkList chunks;
    chunks.push_back(create_chunk(80, 60, 0.3));
    chunks.push_back(create_chunk(85, 55, 0.2));
    
    Vec2d pos{0.0, 0.0};
    
    ThrowAdvice advice = advisor.analyze(chunks, pos, 0.3, 0.05);
    
    EXPECT_GE(advice.expected_certainty_gain, 0.0);
    EXPECT_LE(advice.expected_certainty_gain, 1.0);
}

TEST_F(ThrowAdvisorTest, SuggestedAngleInRange) {
    ChunkList chunks;
    chunks.push_back(create_chunk(100, 50, 0.4));
    
    Vec2d pos{0.0, 0.0};
    
    ThrowAdvice advice = advisor.analyze(chunks, pos, 0.4, 0.05);
    
    if (advice.should_throw) {
        EXPECT_GE(advice.suggested_angle, -180.0);
        EXPECT_LE(advice.suggested_angle, 180.0);
    }
}
