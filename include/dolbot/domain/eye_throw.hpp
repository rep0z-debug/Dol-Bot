#pragma once

#include <cmath>
#include <optional>
#include <string>
#include "dolbot/core/coords.hpp"
#include "dolbot/core/config.hpp"

namespace dolbot::domain {

enum class ThrowType {
    Normal,
    Boat,
    Manual,
    Nether
};

enum class Dimension {
    Overworld,
    Nether,
    End
};

enum class BoatState {
    None,
    Measuring,
    Valid,
    Error
};

struct BoatAngleInfo {
    double raw_angle = 0.0;
    double snapped_angle = 0.0;
    double snap_error = 0.0;
    bool is_positive_grid = true;
    double grid_size = 1.40625;
    int grid_index = 0;
    bool sensitivity_matched = false;
    
    [[nodiscard]] bool within_tolerance(double limit) const {
        if (sensitivity_matched) return true;
        return std::abs(snap_error) <= limit;
    }
};

class EyeThrow {
public:
    core::Vec2d position;
    double horizontal_angle = 0.0;
    double vertical_angle = 0.0;
    double correction = 0.0;
    ThrowType type = ThrowType::Normal;
    Dimension dimension = Dimension::Overworld;
    bool is_boat_mode = false;
    double boat_error_limit = 0.03;
    double boat_sensitivity = 0.0;
    bool use_boat_sensitivity = false;
    bool use_subpixel = false;
    double subpixel_adjustment = 0.0;
    double crosshair_correction = 0.0;
    
    EyeThrow() = default;
    
    EyeThrow(double x, double z, double h_angle, double v_angle = 0.0)
        : position(x, z)
        , horizontal_angle(h_angle)
        , vertical_angle(v_angle)
    {}
    
    [[nodiscard]] double corrected_angle() const {
        double a = horizontal_angle;
        
        if (is_boat_mode) {
            a = compute_boat_snapped_angle(a);
        }
        
        a += crosshair_correction;
        
        a = apply_packet_rounding_correction(a);
        
        a += correction;
        
        if (use_subpixel) {
            a += subpixel_adjustment;
        }
        
        return a;
    }
    
    [[nodiscard]] double raw_corrected_angle() const {
        double a = horizontal_angle;
        
        if (is_boat_mode) {
            a = compute_boat_snapped_angle(a);
        }
        
        return a + correction;
    }
    
    [[nodiscard]] double compute_boat_snapped_angle(double angle) const {
        if (use_boat_sensitivity && boat_sensitivity > 0.0) {
            return compute_sensitivity_snapped_angle(angle);
        }
        double normalized = core::coords::normalize_angle(angle);
        if (normalized >= 0) {
            return std::round(angle / 1.40625) * 1.40625;
        } else {
            return std::round(angle / 0.140625) * 0.140625;
        }
    }
    
    [[nodiscard]] double compute_sensitivity_snapped_angle(double angle) const {
        double sens_factor = sensitivity_to_angle_increment(boat_sensitivity);
        double normalized = core::coords::normalize_angle(angle);
        double grid_size = (normalized >= 0) ? 1.40625 : 0.140625;
        
        int base_grid_index = static_cast<int>(std::round(angle / grid_size));
        double base_grid_angle = base_grid_index * grid_size;
        
        double best_angle = base_grid_angle;
        double best_diff = std::abs(angle - base_grid_angle);
        
        for (int offset = -2; offset <= 2; ++offset) {
            double candidate_grid = (base_grid_index + offset) * grid_size;
            int sens_steps = static_cast<int>(std::round((angle - candidate_grid) / sens_factor));
            for (int s = -2; s <= 2; ++s) {
                double candidate = candidate_grid + (sens_steps + s) * sens_factor;
                double diff = std::abs(candidate - angle);
                if (diff < best_diff) {
                    best_diff = diff;
                    best_angle = candidate;
                }
            }
        }
        
        return best_angle;
    }
    
    [[nodiscard]] double sensitivity_to_angle_increment(double sens) const {
        double raw = sens * 0.6 + 0.2;
        double cubed = raw * raw * raw;
        return cubed * 8.0 * 0.15;
    }
    
    [[nodiscard]] double apply_packet_rounding_correction(double alpha) const {
        return alpha - 0.000824 * std::sin((alpha + 45.0) * core::coords::PI / 180.0);
    }
    
    [[nodiscard]] BoatAngleInfo compute_boat_info() const {
        BoatAngleInfo info;
        info.raw_angle = horizontal_angle;
        
        double normalized = core::coords::normalize_angle(horizontal_angle);
        info.is_positive_grid = (normalized >= 0);
        info.grid_size = info.is_positive_grid ? 1.40625 : 0.140625;
        
        info.snapped_angle = compute_boat_snapped_angle(horizontal_angle);
        info.snap_error = horizontal_angle - info.snapped_angle;
        info.grid_index = static_cast<int>(std::round(horizontal_angle / info.grid_size));
        
        if (use_boat_sensitivity && boat_sensitivity > 0.0) {
            double sens_tolerance = sensitivity_to_angle_increment(boat_sensitivity) * 0.6;
            info.sensitivity_matched = std::abs(info.snap_error) <= sens_tolerance;
        }
        
        return info;
    }
    
    [[nodiscard]] core::Vec2d overworld_position() const {
        if (dimension == Dimension::Nether) {
            return core::coords::nether_to_overworld(position);
        }
        return position;
    }
    
    [[nodiscard]] double x_overworld() const {
        return overworld_position().x;
    }
    
    [[nodiscard]] double z_overworld() const {
        return overworld_position().z;
    }
    
    [[nodiscard]] double angle_radians() const {
        return core::coords::to_radians(corrected_angle());
    }
    
    [[nodiscard]] core::Vec2d direction() const {
        double rad = angle_radians();
        return {-std::sin(rad), std::cos(rad)};
    }
    
    [[nodiscard]] bool is_looking_down() const {
        return vertical_angle > 31.0;
    }
    
    [[nodiscard]] bool is_boat_error() const {
        if (!is_boat_mode) return false;
        auto info = compute_boat_info();
        return !info.within_tolerance(boat_error_limit);
    }
    
    [[nodiscard]] BoatState get_boat_state() const {
        if (!is_boat_mode) return BoatState::None;
        return is_boat_error() ? BoatState::Error : BoatState::Valid;
    }
    
    [[nodiscard]] std::string format_angle() const {
        double a = corrected_angle();
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f", a);
        return std::string(buf);
    }
    
    [[nodiscard]] std::string format_correction() const {
        if (std::abs(correction) < 1e-6) return "";
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%+.2f", correction);
        return std::string(buf);
    }
    
    void apply_correction(double delta) {
        correction += delta;
    }
    
    void reset_correction() {
        correction = 0.0;
    }
    
    void set_subpixel_adjustment(double height_resolution) {
        use_subpixel = true;
        subpixel_adjustment = 0.0;
        if (height_resolution > 0) {
            double fov_factor = 70.0 / height_resolution;
            subpixel_adjustment = fov_factor * 0.001;
        }
    }
    
    void set_crosshair_correction(double correction_value) {
        crosshair_correction = correction_value;
    }
    
    [[nodiscard]] ThrowType effective_type() const {
        if (is_boat_mode) return ThrowType::Boat;
        return type;
    }
    
    [[nodiscard]] double get_standard_deviation(double normal_std, double boat_std, double manual_std) const {
        if (is_boat_mode) return boat_std;
        if (type == ThrowType::Manual) return manual_std;
        return normal_std;
    }
};

class EyeThrowBuilder {
public:
    EyeThrowBuilder& at_position(double x, double z) {
        throw_.position = {x, z};
        return *this;
    }
    
    EyeThrowBuilder& at_position(const core::Vec2d& pos) {
        throw_.position = pos;
        return *this;
    }
    
    EyeThrowBuilder& with_angle(double horizontal, double vertical = 0.0) {
        throw_.horizontal_angle = horizontal;
        throw_.vertical_angle = vertical;
        return *this;
    }
    
    EyeThrowBuilder& in_dimension(Dimension dim) {
        throw_.dimension = dim;
        return *this;
    }
    
    EyeThrowBuilder& of_type(ThrowType type) {
        throw_.type = type;
        return *this;
    }
    
    EyeThrowBuilder& with_boat_mode(bool enabled, double error_limit = 0.03, double sensitivity = 0.0, bool use_sensitivity = false) {
        throw_.is_boat_mode = enabled;
        throw_.boat_error_limit = error_limit;
        throw_.boat_sensitivity = sensitivity;
        throw_.use_boat_sensitivity = use_sensitivity;
        if (enabled) throw_.type = ThrowType::Boat;
        return *this;
    }
    
    EyeThrowBuilder& with_subpixel(double height_resolution) {
        throw_.set_subpixel_adjustment(height_resolution);
        return *this;
    }
    
    EyeThrowBuilder& with_crosshair_correction(double correction) {
        throw_.crosshair_correction = correction;
        return *this;
    }
    
    EyeThrow build() const {
        return throw_;
    }

private:
    EyeThrow throw_;
};

inline std::optional<EyeThrow> parse_f3c_throw(const std::string& input, double crosshair_correction = 0.0);

}
