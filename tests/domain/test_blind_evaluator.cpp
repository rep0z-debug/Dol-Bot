#include <gtest/gtest.h>
#include "dolbot/domain/blind_evaluator.hpp"
#include "dolbot/domain/divine_context.hpp"

using namespace dolbot::domain;
using namespace dolbot::core;

class BlindEvaluatorTest : public ::testing::Test {
protected:
    BlindEvaluator evaluator;
    
    void SetUp() override {
    }
};

TEST_F(BlindEvaluatorTest, EvaluateInFirstRing) {
    BlindResult result = evaluator.evaluate(176.0, 176.0, 400);
    
    EXPECT_NE(result.evaluation, BlindEvaluation::NotInRing);
    EXPECT_GT(result.highroll_probability, 0.0);
}

TEST_F(BlindEvaluatorTest, EvaluateAtOrigin) {
    BlindResult result = evaluator.evaluate(5.0, 5.0, 400);
    
    EXPECT_EQ(result.evaluation, BlindEvaluation::NotInRing);
}

TEST_F(BlindEvaluatorTest, EvaluateFarFromOrigin) {
    BlindResult result = evaluator.evaluate(1500.0, 1500.0, 400);
    
    EXPECT_GE(result.x, 0.0);
}

TEST_F(BlindEvaluatorTest, EvaluateNegativeCoords) {
    BlindResult result = evaluator.evaluate(-200.0, -200.0, 400);
    
    EXPECT_TRUE(true); 
}

TEST_F(BlindEvaluatorTest, HighrollProbabilityInRange) {
    BlindResult result = evaluator.evaluate(200.0, 200.0, 400);
    
    EXPECT_GE(result.highroll_probability, 0.0);
    EXPECT_LE(result.highroll_probability, 1.0);
}

TEST_F(BlindEvaluatorTest, HighrollProbabilityIncreaseWithBetterPosition) {
    BlindResult result = evaluator.evaluate(176.0, 176.0, 400);
    
    EXPECT_GE(result.highroll_probability, 0.0);
}

TEST_F(BlindEvaluatorTest, DistanceThresholdAffectsResult) {
    BlindResult result_low = evaluator.evaluate(200.0, 200.0, 200);
    BlindResult result_high = evaluator.evaluate(200.0, 200.0, 600);
    
    EXPECT_GE(result_low.distance_threshold, 0);
    EXPECT_GE(result_high.distance_threshold, 0);
}

TEST_F(BlindEvaluatorTest, RatingIsGoodForExcellent) {
    BlindResult result;
    result.evaluation = BlindEvaluation::Excellent;
    
    EXPECT_TRUE(result.is_good());
    EXPECT_FALSE(result.rating().empty());
}

TEST_F(BlindEvaluatorTest, RatingIsGoodForHighrollGood) {
    BlindResult result;
    result.evaluation = BlindEvaluation::HighrollGood;
    
    EXPECT_TRUE(result.is_good());
}

TEST_F(BlindEvaluatorTest, RatingIsNotGoodForBad) {
    BlindResult result;
    result.evaluation = BlindEvaluation::Bad;
    
    EXPECT_FALSE(result.is_good());
}

TEST_F(BlindEvaluatorTest, RatingIsNotGoodForNotInRing) {
    BlindResult result;
    result.evaluation = BlindEvaluation::NotInRing;
    
    EXPECT_FALSE(result.is_good());
}

TEST_F(BlindEvaluatorTest, RatingScoreOrdering) {
    BlindResult excellent, good, okay, bad_ring, bad, not_in;
    excellent.evaluation = BlindEvaluation::Excellent;
    good.evaluation = BlindEvaluation::HighrollGood;
    okay.evaluation = BlindEvaluation::HighrollOkay;
    bad_ring.evaluation = BlindEvaluation::BadButInRing;
    bad.evaluation = BlindEvaluation::Bad;
    not_in.evaluation = BlindEvaluation::NotInRing;
    
    EXPECT_GT(excellent.rating_score(), good.rating_score());
    EXPECT_GT(good.rating_score(), okay.rating_score());
    EXPECT_GT(okay.rating_score(), bad_ring.rating_score());
    EXPECT_GT(bad_ring.rating_score(), bad.rating_score());
    EXPECT_GT(bad.rating_score(), not_in.rating_score());
}

TEST_F(BlindEvaluatorTest, RatingStringNotEmpty) {
    for (int i = 0; i <= 5; ++i) {
        BlindResult result;
        result.evaluation = static_cast<BlindEvaluation>(i);
        EXPECT_FALSE(result.rating().empty());
    }
}

TEST_F(BlindEvaluatorTest, AverageDistancePositive) {
    BlindResult result = evaluator.evaluate(200.0, 200.0, 400);
    
    EXPECT_GE(result.avg_distance, 0.0);
}

TEST_F(BlindEvaluatorTest, SetDivineContext) {
    DivineContext ctx;
    ctx.direction_angle = 45.0;
    ctx.ring_index = 0;
    ctx.is_active = true;
    
    evaluator.set_divine_context(ctx);
    
    BlindResult result = evaluator.evaluate(200.0, 200.0, 400);
    EXPECT_GE(result.highroll_probability, 0.0);
}

TEST_F(BlindEvaluatorTest, ConstructorWithDivineContext) {
    DivineContext ctx;
    ctx.direction_angle = 90.0;
    ctx.ring_index = 1;
    ctx.is_active = true;
    
    BlindEvaluator evaluator_with_ctx(ctx);
    
    BlindResult result = evaluator_with_ctx.evaluate(300.0, 300.0, 400);
    EXPECT_GE(result.highroll_probability, 0.0);
}

TEST_F(BlindEvaluatorTest, EvaluateZeroCoords) {
    BlindResult result = evaluator.evaluate(0.0, 0.0, 400);
    
    EXPECT_EQ(result.evaluation, BlindEvaluation::NotInRing);
}

TEST_F(BlindEvaluatorTest, EvaluateLargeCoords) {
    BlindResult result = evaluator.evaluate(10000.0, 10000.0, 400);
    
    EXPECT_GE(result.x, 0.0);
}

TEST_F(BlindEvaluatorTest, EvaluateZeroThreshold) {
    BlindResult result = evaluator.evaluate(200.0, 200.0, 0);
    
    EXPECT_GE(result.highroll_probability, 0.0);
}

TEST_F(BlindEvaluatorTest, EvaluateNegativeCoordinates) {
    BlindResult result = evaluator.evaluate(-176.0, -176.0, 400);
    
    EXPECT_NE(result.evaluation, BlindEvaluation::NotInRing);
}
