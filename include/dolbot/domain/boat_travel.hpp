#pragma once

#include <cmath>
#include <optional>
#include "dolbot/core/coords.hpp"

namespace dolbot::domain {

class BoatTravel {
public:
    static constexpr double BOAT_INCREMENT = 1.40625;
    static constexpr double BOAT_INCREMENT_SMALL = 0.140625;
    
    struct BoatStateInfo {
        double angle;
        bool is_valid;
        bool is_entering;
        double snap_error;
        int grid_index;
    };
    
    static double get_precise_boat_angle(double alpha, double sensitivity, double boat_angle) {
        double pre_multiplier = sensitivity * 0.6 + 0.2;
        pre_multiplier = pre_multiplier * pre_multiplier * pre_multiplier * 8.0;
        double min_inc = pre_multiplier * 0.15;
        return boat_angle + std::round((alpha - boat_angle) / min_inc) * min_inc;
    }
    
    static double get_min_angle_increment(double sensitivity) {
        double pre_multiplier = sensitivity * 0.6 + 0.2;
        return std::pow(pre_multiplier, 3) * 8.0 * 0.15;
    }

    static bool is_valid_boat_angle(double angle, double error_limit) {
        if (std::abs(angle) > 360.0) return false;
        
        double candidate = (angle >= 0) 
            ? std::round(angle / BOAT_INCREMENT) * BOAT_INCREMENT 
            : std::round(angle / BOAT_INCREMENT_SMALL) * BOAT_INCREMENT_SMALL;
            
        double rounded = std::round(candidate * 100.0) / 100.0;
        return std::abs(rounded - angle) <= error_limit;
    }
    
    static bool is_valid_boat_angle_with_sensitivity(double angle, double sensitivity, double error_limit) {
        double min_inc = get_min_angle_increment(sensitivity);
        double snapped = std::round(angle / min_inc) * min_inc;
        return std::abs(snapped - angle) <= error_limit;
    }

    static std::optional<double> snap_angle(double angle) {
        if (std::abs(angle) > 360.0) return std::nullopt;
        
        double candidate = (angle >= 0) 
            ? std::round(angle / BOAT_INCREMENT) * BOAT_INCREMENT 
            : std::round(angle / BOAT_INCREMENT_SMALL) * BOAT_INCREMENT_SMALL;
            
        return candidate;
    }
    
    static std::optional<double> snap_angle_with_sensitivity(double angle, double sensitivity, double boat_angle) {
        if (std::abs(angle) > 360.0) return std::nullopt;
        return get_precise_boat_angle(angle, sensitivity, boat_angle);
    }
    
    static double get_snap_error(double angle) {
        auto snapped = snap_angle(angle);
        if (!snapped) return 0.0;
        return angle - *snapped;
    }
    
    static int get_grid_index(double angle) {
        double grid_size = (angle >= 0) ? BOAT_INCREMENT : BOAT_INCREMENT_SMALL;
        return static_cast<int>(std::round(angle / grid_size));
    }

    static double reduce_angle_mod360(double current_boat_angle, double player_angle, double sensitivity) {
        double pre_multiplier = sensitivity * 0.6 + 0.2;
        pre_multiplier = std::pow(pre_multiplier, 3) * 8.0;
        double min_inc = pre_multiplier * 0.15;
        
        double diff = player_angle - current_boat_angle;
        double rounded_diff = std::round(diff / min_inc) * min_inc;
        double true_angle = current_boat_angle + rounded_diff;
        
        double change = true_angle - std::fmod(true_angle, 360.0);
        return current_boat_angle - change;
    }
    
    static BoatStateInfo compute_boat_state(double angle, double error_limit = 0.03) {
        BoatStateInfo info;
        info.angle = angle;
        info.grid_index = get_grid_index(angle);
        info.snap_error = get_snap_error(angle);
        info.is_valid = is_valid_boat_angle(angle, error_limit);
        info.is_entering = false;
        return info;
    }
    
    static BoatStateInfo compute_boat_state_with_sensitivity(
        double angle, 
        double sensitivity, 
        double boat_angle,
        double error_limit = 0.03
    ) {
        BoatStateInfo info;
        info.angle = angle;
        info.grid_index = get_grid_index(angle);
        
        double snapped = get_precise_boat_angle(angle, sensitivity, boat_angle);
        info.snap_error = angle - snapped;
        info.is_valid = std::abs(info.snap_error) <= error_limit;
        info.is_entering = false;
        
        return info;
    }
};

}
