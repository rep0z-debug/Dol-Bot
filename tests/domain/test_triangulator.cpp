#include <gtest/gtest.h>
#include "dolbot/domain/triangulator.hpp"
#include "dolbot/domain/eye_throw.hpp"

using namespace dolbot::domain;
using namespace dolbot::core;

class TriangulatorTest : public ::testing::Test {
protected:
    Triangulator calc;
    
    void SetUp() override {
        calc.set_settings(0.05, 0.001, 0.03, true, McVersion::V1_13_to_1_18);
    }
};

TEST_F(TriangulatorTest, EmptyReturnsNoResult) {
    EXPECT_FALSE(calc.result().has_result());
    EXPECT_EQ(calc.throw_count(), 0);
}

TEST_F(TriangulatorTest, SingleThrowGivesResult) {
    EyeThrow t;
    t.position = {100.0, 200.0};
    t.horizontal_angle = 45.0;
    
    calc.add_throw(t);
    
    EXPECT_TRUE(calc.result().has_result());
    EXPECT_EQ(calc.throw_count(), 1);
}

TEST_F(TriangulatorTest, TwoThrowsImprovesCertainty) {
    EyeThrow t1, t2;
    t1.position = {100.0, 200.0};
    t1.horizontal_angle = 30.0;
    
    calc.add_throw(t1);
    double cert1 = calc.result().top_certainty();
    
    t2.position = {500.0, -100.0};
    t2.horizontal_angle = 60.0;
    
    calc.add_throw(t2);
    double cert2 = calc.result().top_certainty();
    
    EXPECT_GT(cert2, cert1);
}

TEST_F(TriangulatorTest, UndoRemovesThrow) {
    EyeThrow t;
    t.position = {100.0, 200.0};
    t.horizontal_angle = 45.0;
    
    calc.add_throw(t);
    EXPECT_EQ(calc.throw_count(), 1);
    
    calc.undo_last();
    EXPECT_EQ(calc.throw_count(), 0);
}

TEST_F(TriangulatorTest, ResetClearsAll) {
    EyeThrow t;
    t.position = {100.0, 200.0};
    t.horizontal_angle = 45.0;
    
    calc.add_throw(t);
    calc.add_throw(t);
    calc.reset();
    
    EXPECT_EQ(calc.throw_count(), 0);
    EXPECT_FALSE(calc.result().has_result());
}

TEST_F(TriangulatorTest, LockPreventsModifications) {
    EyeThrow t;
    t.position = {100.0, 200.0};
    t.horizontal_angle = 45.0;
    
    calc.add_throw(t);
    calc.lock();
    
    calc.add_throw(t);
    EXPECT_EQ(calc.throw_count(), 1);
    
    calc.undo_last();
    EXPECT_EQ(calc.throw_count(), 1);
    
    calc.unlock();
    calc.undo_last();
    EXPECT_EQ(calc.throw_count(), 0);
}
