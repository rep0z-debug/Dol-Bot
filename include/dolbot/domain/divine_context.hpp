#pragma once

#include <cmath>
#include <optional>
#include <array>
#include "dolbot/core/coords.hpp"

namespace dolbot::domain {

struct Fossil {
    int x;
    
    static constexpr int MIN_X = 0;
    static constexpr int MAX_X = 15;
    
    [[nodiscard]] bool is_valid() const {
        return x >= MIN_X && x <= MAX_X;
    }
    
    [[nodiscard]] double sector_start_angle() const {
        return x * (2.0 * core::coords::PI / 16.0) - core::coords::PI;
    }
    
    [[nodiscard]] double sector_end_angle() const {
        return (x + 1) * (2.0 * core::coords::PI / 16.0) - core::coords::PI;
    }
    
    [[nodiscard]] double sector_center_angle() const {
        return (sector_start_angle() + sector_end_angle()) / 2.0;
    }
};

class DivineContext {
public:
    DivineContext() = default;
    
    explicit DivineContext(const Fossil& fossil) : fossil_(fossil) {}
    
    void set_fossil(const Fossil& fossil) {
        fossil_ = fossil;
    }
    
    void clear() {
        fossil_ = std::nullopt;
    }
    
    [[nodiscard]] bool has_divine() const {
        return fossil_.has_value() && fossil_->is_valid();
    }
    
    [[nodiscard]] std::optional<Fossil> get_fossil() const {
        return fossil_;
    }
    
    [[nodiscard]] double density_at_angle_before_snapping(double phi) const {
        if (!has_divine()) {
            return 1.0 / (2.0 * core::coords::PI);
        }
        
        double phi_start_base = fossil_->sector_start_angle();
        double phi_end_base = fossil_->sector_end_angle();
        double sector_width = phi_end_base - phi_start_base;
        
        double normalized_phi = normalize_phi(phi);
        
        for (int k = 0; k < 3; ++k) {
            double offset = k * (2.0 * core::coords::PI / 3.0);
            
            double s_start = normalize_phi(phi_start_base + offset);
            double s_end = normalize_phi(phi_end_base + offset);
            
            bool inside = false;
            if (s_start < s_end) {
                inside = (normalized_phi >= s_start && normalized_phi <= s_end);
            } else {
                inside = (normalized_phi >= s_start || normalized_phi <= s_end);
            }
            
            if (inside) {
                return 1.0 / (3.0 * sector_width);
            }
        }
        
        return 0.0;
    }
    
    [[nodiscard]] double relative_density() const {
        if (!has_divine()) return 1.0;
        return 16.0 / 3.0;
    }
    
    struct ClosestCoords {
        double x;
        double z;
    };
    
    [[nodiscard]] ClosestCoords get_closest_coords(double x, double z, double target_distance) const {
        if (!has_divine()) {
            double r = std::sqrt(x * x + z * z);
            if (r < 1.0) return {0.0, target_distance};
            return {x * target_distance / r, z * target_distance / r};
        }
        
        double target_phi_base = fossil_->sector_center_angle();
        double best_dist_sq = std::numeric_limits<double>::max();
        double best_phi = target_phi_base;
        
        double input_phi = std::atan2(-x, z);
        
        for (int k = 0; k < 3; ++k) {
            double sector_center = normalize_phi(target_phi_base + k * (2.0 * core::coords::PI / 3.0));
            
            double diff = std::abs(normalize_phi(input_phi - sector_center));
            if (diff > core::coords::PI) diff = 2.0 * core::coords::PI - diff;
            
            if (diff < best_dist_sq) {
                best_dist_sq = diff;
                best_phi = sector_center;
            }
        }
        
        double opt_x = -target_distance * std::sin(best_phi);
        double opt_z = target_distance * std::cos(best_phi);
        
        return {opt_x, opt_z};
    }

private:
    std::optional<Fossil> fossil_;
    
    static double normalize_phi(double phi) {
        while (phi < -core::coords::PI) phi += 2.0 * core::coords::PI;
        while (phi > core::coords::PI) phi -= 2.0 * core::coords::PI;
        return phi;
    }
};

}
