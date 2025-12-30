#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <optional>
#include "dolbot/core/coords.hpp"
#include "dolbot/domain/world_chunk.hpp"
#include "dolbot/domain/eye_throw.hpp"
#include "dolbot/domain/probability_posterior.hpp"

namespace dolbot::domain {

struct ThrowAdvice {
    double suggested_angle = 0.0;
    double expected_certainty_gain = 0.0;
    std::string direction_description;
    bool should_throw = false;
    double current_certainty = 0.0;
    double expected_certainty = 0.0;
    double optimal_distance = 0.0;
    double perpendicular_angle = 0.0;
    core::Vec2d suggested_position;
};

struct SecondThrowAdvice {
    core::Vec2d optimal_position;
    double distance_to_travel = 0.0;
    double travel_angle = 0.0;
    double expected_cross_product = 0.0;
    std::string quality_rating;
    bool is_perpendicular = false;
};

class ThrowAdvisor {
public:
    ThrowAdvisor() = default;
    
    ThrowAdvice analyze(
        const ChunkList& top_predictions,
        const core::Vec2d& last_throw_pos,
        double current_certainty,
        double std_deviation
    ) {
        ThrowAdvice advice;
        advice.current_certainty = current_certainty;
        
        if (top_predictions.empty() || current_certainty >= 0.95) {
            advice.should_throw = false;
            advice.direction_description = "High confidence, proceed to stronghold";
            return advice;
        }
        
        if (current_certainty >= 0.85) {
            advice.should_throw = false;
            advice.direction_description = "Good confidence, throw optional";
            return advice;
        }
        
        double best_angle = 0.0;
        double best_gain = 0.0;
        
        for (int angle_deg = 0; angle_deg < 360; angle_deg += 15) {
            double test_angle = static_cast<double>(angle_deg);
            double gain = estimate_certainty_gain(
                top_predictions, last_throw_pos, test_angle, std_deviation
            );
            
            if (gain > best_gain) {
                best_gain = gain;
                best_angle = test_angle;
            }
        }
        
        for (double delta = -10; delta <= 10; delta += 2) {
            double test_angle = best_angle + delta;
            double gain = estimate_certainty_gain(
                top_predictions, last_throw_pos, test_angle, std_deviation
            );
            
            if (gain > best_gain) {
                best_gain = gain;
                best_angle = test_angle;
            }
        }
        
        advice.suggested_angle = core::coords::normalize_angle(best_angle);
        advice.expected_certainty_gain = best_gain;
        advice.expected_certainty = std::min(1.0, current_certainty + best_gain);
        advice.should_throw = (best_gain > 0.1);
        advice.optimal_distance = 300.0;
        
        double rad = core::coords::to_radians(advice.suggested_angle);
        advice.suggested_position = {
            last_throw_pos.x - std::sin(rad) * advice.optimal_distance,
            last_throw_pos.z + std::cos(rad) * advice.optimal_distance
        };
        
        advice.direction_description = format_direction(advice.suggested_angle);
        
        return advice;
    }
    
    SecondThrowAdvice analyze_second_throw(
        const EyeThrow& first_throw,
        const WorldChunk& predicted_chunk,
        core::McVersion version
    ) {
        SecondThrowAdvice advice;
        
        core::Vec2d first_pos = first_throw.overworld_position();
        core::Vec2d first_dir = first_throw.direction();
        
        int sh_x = predicted_chunk.stronghold_x(version);
        int sh_z = predicted_chunk.stronghold_z(version);
        core::Vec2d stronghold_pos{static_cast<double>(sh_x), static_cast<double>(sh_z)};
        
        core::Vec2d perpendicular{-first_dir.z, first_dir.x};
        
        core::Vec2d to_stronghold = stronghold_pos - first_pos;
        double dist_to_stronghold = to_stronghold.length();
        
        double optimal_travel_dist = std::min(300.0, dist_to_stronghold * 0.4);
        
        core::Vec2d left_pos = first_pos + perpendicular * optimal_travel_dist;
        core::Vec2d right_pos = first_pos - perpendicular * optimal_travel_dist;
        
        double left_dist = (stronghold_pos - left_pos).length();
        double right_dist = (stronghold_pos - right_pos).length();
        
        advice.optimal_position = (left_dist < right_dist) ? left_pos : right_pos;
        advice.distance_to_travel = optimal_travel_dist;
        
        core::Vec2d travel_vec = advice.optimal_position - first_pos;
        advice.travel_angle = core::coords::angle_from_coords(travel_vec.x, travel_vec.z);
        
        core::Vec2d second_dir = (stronghold_pos - advice.optimal_position).normalized();
        advice.expected_cross_product = std::abs(first_dir.x * second_dir.z - first_dir.z * second_dir.x);
        
        advice.is_perpendicular = (advice.expected_cross_product > 0.5);
        
        if (advice.expected_cross_product > 0.7) {
            advice.quality_rating = "Excellent";
        } else if (advice.expected_cross_product > 0.5) {
            advice.quality_rating = "Good";
        } else if (advice.expected_cross_product > 0.3) {
            advice.quality_rating = "Acceptable";
        } else {
            advice.quality_rating = "Poor";
        }
        
        return advice;
    }
    
    std::optional<core::Vec2d> compute_optimal_second_position(
        const std::vector<EyeThrow>& throws,
        const std::vector<PredictionResult>& predictions,
        core::McVersion version
    ) {
        if (throws.empty() || predictions.empty()) {
            return std::nullopt;
        }
        
        const auto& first_throw = throws[0];
        const auto& best_pred = predictions[0];
        
        if (best_pred.certainty > 0.9) {
            return std::nullopt;
        }
        
        auto advice = analyze_second_throw(first_throw, best_pred.chunk, version);
        
        if (advice.expected_cross_product < 0.2) {
            core::Vec2d dir = first_throw.direction();
            core::Vec2d perpendicular{-dir.z, dir.x};
            return first_throw.overworld_position() + perpendicular * 200.0;
        }
        
        return advice.optimal_position;
    }
    
private:
    double estimate_certainty_gain(
        const ChunkList& predictions,
        const core::Vec2d& from_pos,
        double travel_angle,
        double std_dev
    ) {
        double travel_dist = 300.0;
        double rad = core::coords::to_radians(travel_angle);
        core::Vec2d new_pos = {
            from_pos.x - std::sin(rad) * travel_dist,
            from_pos.z + std::cos(rad) * travel_dist
        };
        
        double total_separation = 0.0;
        int count = 0;
        
        for (size_t i = 0; i < std::min(size_t(5), predictions.size()); ++i) {
            for (size_t j = i + 1; j < std::min(size_t(5), predictions.size()); ++j) {
                double angle_i = calculate_angle_to_chunk(new_pos, predictions[i]);
                double angle_j = calculate_angle_to_chunk(new_pos, predictions[j]);
                
                double sep = std::abs(core::coords::normalize_angle(angle_i - angle_j));
                total_separation += sep;
                ++count;
            }
        }
        
        if (count == 0) return 0.0;
        
        double avg_separation = total_separation / count;
        double normalized_sep = std::min(90.0, avg_separation) / 90.0;
        
        double sigma_factor = std::max(0.01, std_dev);
        double certainty_gain = normalized_sep * 0.3 / sigma_factor;
        
        return std::min(0.5, certainty_gain);
    }
    
    double calculate_angle_to_chunk(const core::Vec2d& pos, const WorldChunk& chunk) {
        double dx = chunk.pos.x * 16 + 8 - pos.x;
        double dz = chunk.pos.z * 16 + 8 - pos.z;
        return core::coords::angle_from_coords(dx, dz);
    }
    
    std::string format_direction(double angle) {
        angle = core::coords::normalize_angle(angle);
        
        if (angle >= -22.5 && angle < 22.5) return "North";
        if (angle >= 22.5 && angle < 67.5) return "Northeast";
        if (angle >= 67.5 && angle < 112.5) return "East";
        if (angle >= 112.5 && angle < 157.5) return "Southeast";
        if (angle >= 157.5 || angle < -157.5) return "South";
        if (angle >= -157.5 && angle < -112.5) return "Southwest";
        if (angle >= -112.5 && angle < -67.5) return "West";
        return "Northwest";
    }
};

}
