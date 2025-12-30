#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include "dolbot/core/coords.hpp"
#include "dolbot/domain/triangulator.hpp"
#include "dolbot/domain/eye_throw.hpp"
#include "dolbot/domain/fortress_ring.hpp"

using namespace dolbot::domain;
using namespace dolbot::core;

void test_coords() {
    std::cout << "Testing Coordinates... ";
    Vec2d v{3.0, 4.0};
    assert(std::abs(v.length() - 5.0) < 1e-9);
    
    auto n = v.normalized();
    assert(std::abs(n.length() - 1.0) < 1e-9);
    
    assert(std::abs(coords::normalize_angle(370.0) - 10.0) < 1e-9);
    assert(std::abs(coords::normalize_angle(-370.0) - -10.0) < 1e-9);
    std::cout << "OK" << std::endl;
}

void test_ring_system() {
    std::cout << "Testing Ring System... ";
    const auto& rings = RingSystem::instance().all();
    assert(rings.size() == 8);
    assert(rings[0].stronghold_count == 3);
    
    auto [r1, r2] = RingSystem::instance().find_closest_rings(0, 0);
    assert(r1->ring_index == 0);
    std::cout << "OK" << std::endl;
}

void test_triangulation() {
    std::cout << "Testing Triangulation... ";
    
    Triangulator calc;
    calc.set_settings(0.05, 0.001, 0.03, true, McVersion::V1_13_to_1_18);
    
    EyeThrow t1;
    t1.position = {0, 0};
    t1.horizontal_angle = -45.0; 
    calc.add_throw(t1);
    
    EyeThrow t2;
    t2.position = {0, 2000};
    t2.horizontal_angle = -135.0;
    calc.add_throw(t2);
    
    assert(calc.result().has_result());
    
    const auto* best = calc.result().best();
    assert(best != nullptr);
    
    double pred_x = best->chunk.stronghold_x();
    double pred_z = best->chunk.stronghold_z();
    
    double dist = std::sqrt(std::pow(pred_x - 1000, 2) + std::pow(pred_z - 1000, 2));
    
    bool result_valid = dist < 2000.0; 
    
    if (result_valid) {
        std::cout << "OK (Prediction: " << pred_x << ", " << pred_z << ")" << std::endl;
    } else {
        std::cout << "FAILED (Prediction: " << pred_x << ", " << pred_z << " too far from expected)" << std::endl;
        assert(false);
    }
}

int main() {
    std::cout << "Running Verification Tests..." << std::endl;
    try {
        test_coords();
        test_ring_system();
        test_triangulation();
        std::cout << "\nALL TESTS PASSED - Logic Verified." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\nTEST FAILED: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "\nTEST FAILED: Unknown error" << std::endl;
        return 1;
    }
}
