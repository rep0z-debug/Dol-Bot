#pragma once

#include <cmath>
#include <optional>
#include <vector>
#include <tuple>
#include "dolbot/core/coords.hpp"
#include "dolbot/domain/fortress_ring.hpp"
#include "dolbot/domain/probability_prior.hpp"
#include "dolbot/domain/divine_context.hpp"

namespace dolbot::domain {

enum class BlindEvaluation {
    Excellent,
    HighrollGood,
    HighrollOkay,
    BadButInRing,
    Bad,
    NotInRing
};

struct BlindResult {
    double x = 0.0;
    double z = 0.0;
    double highroll_probability = 0.0;
    int distance_threshold = 0;
    double avg_distance = 0.0;
    double avg_distance_derivative = 0.0;
    double ninetieth_percentile_derivative = 0.0;
    double improvement_direction = 0.0;
    double improvement_distance = 0.0;
    double optimal_highroll_prob = 0.0;
    BlindEvaluation evaluation = BlindEvaluation::NotInRing;
    
    [[nodiscard]] bool is_good() const {
        return evaluation == BlindEvaluation::Excellent || 
               evaluation == BlindEvaluation::HighrollGood;
    }
    
    [[nodiscard]] std::string rating() const {
        switch (evaluation) {
            case BlindEvaluation::Excellent: return "Excellent";
            case BlindEvaluation::HighrollGood: return "Good";
            case BlindEvaluation::HighrollOkay: return "Okay";
            case BlindEvaluation::BadButInRing: return "Poor";
            case BlindEvaluation::Bad: return "Bad";
            case BlindEvaluation::NotInRing: return "Not in ring";
        }
        return "Unknown";
    }
    
    [[nodiscard]] double rating_score() const {
        switch (evaluation) {
            case BlindEvaluation::Excellent: return 1.0;
            case BlindEvaluation::HighrollGood: return 0.9;
            case BlindEvaluation::HighrollOkay: return 0.7;
            case BlindEvaluation::BadButInRing: return 0.5;
            case BlindEvaluation::Bad: return 0.2;
            case BlindEvaluation::NotInRing: return 0.0;
        }
        return 0.0;
    }
};

class BlindEvaluator {
public:
    BlindEvaluator() = default;
    
    explicit BlindEvaluator(const DivineContext& divine_context) 
        : divine_context_(divine_context) {}
    
    void set_divine_context(const DivineContext& ctx) {
        divine_context_ = ctx;
    }
    
    BlindResult evaluate(double nether_x, double nether_z, int distance_threshold = 400) {
        BlindResult result;
        result.x = nether_x;
        result.z = nether_z;
        result.distance_threshold = distance_threshold;
        
        double ow_x = nether_x * 8.0;
        double ow_z = nether_z * 8.0;
        
        result.highroll_probability = compute_highroll_prob(ow_x, ow_z, distance_threshold);
        result.avg_distance = compute_average_distance(nether_x, nether_z) * 16.0;
        
        constexpr int h = 2;
        double phi_p = compute_phi(nether_x, nether_z);
        
        double prob_dx = compute_highroll_prob(ow_x + h, ow_z, distance_threshold);
        double prob_derivative_x = (prob_dx - result.highroll_probability) / h;
        
        double prob_dz = compute_highroll_prob(ow_x, ow_z + h, distance_threshold);
        double prob_derivative_z = (prob_dz - result.highroll_probability) / h;
        
        double prob_derivative = std::sqrt(
            prob_derivative_x * prob_derivative_x + 
            prob_derivative_z * prob_derivative_z
        );
        
        if (result.highroll_probability > 1e-10) {
            result.ninetieth_percentile_derivative = prob_derivative * 
                std::sqrt(0.1 / (2 * result.highroll_probability * result.highroll_probability * result.highroll_probability)) * 
                distance_threshold;
        }
        
        double avg_dist_offset = compute_average_distance(
            nether_x - h * std::sin(phi_p), 
            nether_z + h * std::cos(phi_p)
        ) * 16.0;
        result.avg_distance_derivative = (avg_dist_offset - result.avg_distance) / h;
        
        auto [opt_x, opt_z, opt_prob] = compute_optimal_position(ow_x, ow_z, distance_threshold);
        result.improvement_direction = core::coords::angle_from_coords(opt_x - ow_x, opt_z - ow_z);
        result.improvement_distance = std::sqrt(
            (opt_x - ow_x) * (opt_x - ow_x) + (opt_z - ow_z) * (opt_z - ow_z)
        );
        result.optimal_highroll_prob = opt_prob;
        
        result.evaluation = compute_evaluation(result);
        
        return result;
    }
    
    [[nodiscard]] double compute_highroll_prob(double ow_x, double ow_z, int threshold) {
        int chunk_x = static_cast<int>(ow_x / 16);
        int chunk_z = static_cast<int>(ow_z / 16);
        int search_r = threshold / 16 + 1;
        
        ProbabilityPrior prior(chunk_x, chunk_z, search_r, false, divine_context_);
        
        double probability = 0.0;
        for (const auto& chunk : prior.candidates()) {
            double dx = ow_x - (chunk.pos.x * 16 + 8);
            double dz = ow_z - (chunk.pos.z * 16 + 8);
            if (dx * dx + dz * dz < threshold * threshold) {
                probability += chunk.probability;
            }
        }
        
        return std::min(1.0, probability);
    }

private:
    struct Uniform {
        double a;
        double b;
    };
    
    double compute_average_distance(double nether_x, double nether_z) {
        double chunk_x = nether_x / 2.0;
        double chunk_z = nether_z / 2.0;
        
        auto [ring1, ring2] = RingSystem::instance().find_closest_rings(chunk_x, chunk_z);
        if (!ring1) return 0.0;
        
        double section_angle = ring1->section_angle();
        double section_angle2 = ring2 ? ring2->section_angle() : section_angle;
        double ring_thickness = ring1->outer_radius - ring1->inner_radius;
        double phi0 = compute_phi(nether_x, nether_z);
        
        constexpr double rd_phi = 10.0;
        constexpr double d_r = 20.0;
        
        double d_phi = rd_phi / ring1->inner_radius;
        int n_phi = static_cast<int>(section_angle / d_phi);
        d_phi = section_angle / n_phi;
        
        int n_r = static_cast<int>(ring_thickness / d_r);
        double actual_dr = ring_thickness / n_r;
        
        double integral = 0.0;
        
        for (int i = 0; i < n_phi; ++i) {
            double phi = phi0 - section_angle / 2.0 + i * d_phi;
            
            for (int j = 0; j < n_r; ++j) {
                double r = ring1->inner_radius + (j + 0.5) * actual_dr;
                double d = compute_distance(chunk_x, chunk_z, phi, r);
                
                std::vector<Uniform> other_strongholds;
                other_strongholds.push_back(get_stronghold_distr(chunk_x, chunk_z, phi + section_angle, *ring1));
                other_strongholds.push_back(get_stronghold_distr(chunk_x, chunk_z, phi - section_angle, *ring1));
                
                if (ring2) {
                    int i2 = i % 5 - 2;
                    double phi2 = phi0 + i2 / 5.0 * section_angle2;
                    other_strongholds.push_back(get_stronghold_distr(chunk_x, chunk_z, phi2, *ring2));
                    other_strongholds.push_back(get_stronghold_distr(chunk_x, chunk_z, phi2 + section_angle2, *ring2));
                    other_strongholds.push_back(get_stronghold_distr(chunk_x, chunk_z, phi2 - section_angle2, *ring2));
                }
                
                auto [cumulative_prob, expected_dist] = approx_average_dist(other_strongholds, d);
                integral += (d * (1.0 - cumulative_prob) + expected_dist * cumulative_prob) / (n_phi * n_r);
            }
        }
        
        return integral;
    }
    
    std::tuple<double, double, double> compute_optimal_position(
        double ow_x, double ow_z, int threshold) {
        
        double chunk_x = ow_x / 16.0;
        double chunk_z = ow_z / 16.0;
        
        auto [ring, _] = RingSystem::instance().find_closest_rings(chunk_x, chunk_z);
        if (!ring) {
            return {ow_x, ow_z, 0.0};
        }
        
        double opt_dist = (ring->inner_radius + threshold / 16.0) * 16.0;
        double opt_x = ow_x;
        double opt_z = ow_z;
        double opt_highroll_prob = 0.1;
        
        if (divine_context_.has_divine() && ring->ring_index == 0) {
            auto divine_pos = divine_context_.get_closest_coords(opt_x, opt_z, opt_dist);
            opt_x = divine_pos.x;
            opt_z = divine_pos.z;
            opt_highroll_prob *= divine_context_.relative_density();
        }
        
        double opt_r = std::sqrt(opt_x * opt_x + opt_z * opt_z);
        if (opt_r > 1.0) {
            opt_x *= opt_dist / opt_r;
            opt_z *= opt_dist / opt_r;
        } else {
            opt_z = opt_dist;
        }
        
        return {opt_x, opt_z, opt_highroll_prob};
    }
    
    BlindEvaluation compute_evaluation(const BlindResult& result) {
        double chunk_r = std::sqrt(result.x * result.x + result.z * result.z) / 2.0;
        const FortressRing* ring = RingSystem::instance().find_ring(chunk_r);
        
        if (!ring) {
            return BlindEvaluation::NotInRing;
        }
        
        double prob = result.highroll_probability;
        double opt_prob = result.optimal_highroll_prob;
        
        if (opt_prob > 1e-6 && prob / opt_prob < 0.05) {
            return BlindEvaluation::Bad;
        }
        
        double relative_quality = (opt_prob > 1e-6) ? (prob / opt_prob) : prob;
        
        if (relative_quality > 0.80) {
            return BlindEvaluation::Excellent;
        }
        if (relative_quality > 0.60) {
            return BlindEvaluation::HighrollGood;
        }
        if (relative_quality > 0.40) {
            return BlindEvaluation::HighrollOkay;
        }
        if (relative_quality > 0.15) {
            return BlindEvaluation::BadButInRing;
        }
        
        return BlindEvaluation::Bad;
    }
    
    double compute_phi(double x, double z) {
        return -std::atan2(x, z);
    }
    
    double compute_distance(double x, double z, double phi, double r) {
        double dx = x + r * std::sin(phi);
        double dz = z - r * std::cos(phi);
        return std::sqrt(dx * dx + dz * dz);
    }
    
    Uniform get_stronghold_distr(double x, double z, double phi, const FortressRing& ring) {
        double min_d = compute_distance(x, z, phi, ring.inner_radius);
        double max_d = compute_distance(x, z, phi, ring.outer_radius);
        return min_d < max_d ? Uniform{min_d, max_d} : Uniform{max_d, min_d};
    }
    
    std::pair<double, double> approx_average_dist(const std::vector<Uniform>& distributions, double max_dist) {
        std::vector<double> discontinuities;
        
        for (const auto& u : distributions) {
            if (u.a < max_dist) {
                discontinuities.push_back(u.a);
                if (u.b < max_dist) {
                    discontinuities.push_back(u.b);
                }
            }
        }
        discontinuities.push_back(max_dist);
        std::sort(discontinuities.begin(), discontinuities.end());
        
        double cumulative_prob = 0.0;
        double expected_distance = 0.0;
        
        for (size_t j = 1; j < discontinuities.size(); ++j) {
            double lower = discontinuities[j - 1];
            double upper = discontinuities[j];
            double center = (lower + upper) / 2.0;
            
            double complementary_prob = 1.0;
            double n = 0.0;
            
            for (const auto& u : distributions) {
                if (center > u.a && center < u.b) {
                    complementary_prob *= (u.b - upper) / (u.b - lower);
                    n += (upper - lower) / (u.b - lower);
                }
            }
            
            double prob = (1.0 - cumulative_prob) * (1.0 - complementary_prob);
            cumulative_prob += prob;
            
            double ev = (upper + n * lower) / (n + 1);
            expected_distance += prob * ev;
        }
        
        return {cumulative_prob, cumulative_prob > 0.0 ? expected_distance / cumulative_prob : 0.0};
    }
    
    DivineContext divine_context_;
};

}
