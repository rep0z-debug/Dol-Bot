#pragma once

#include "dolbot/core/config.hpp"

namespace dolbot::domain {

struct StandardDeviationSettings {
    double normal_std = 0.1;
    double boat_std = 0.001;
    double manual_std = 0.03;
    double alt_std = 0.1;
    double crosshair_correction = 0.0;
    double sensitivity_automatic = 0.0;
    double sensitivity_manual = 0.0;
    
    [[nodiscard]] double get_expected_std_for_next_throw(bool is_boat, bool is_manual, bool use_alt) const {
        if (is_boat) return boat_std;
        if (is_manual) return use_alt ? alt_std : manual_std;
        return use_alt ? alt_std : normal_std;
    }
    
    [[nodiscard]] double get_sensitivity(bool has_detailed_position) const {
        return has_detailed_position ? sensitivity_automatic : sensitivity_manual;
    }
    
    static StandardDeviationSettings default_settings() {
        return StandardDeviationSettings{};
    }
    
    static StandardDeviationSettings tight_settings() {
        StandardDeviationSettings s;
        s.normal_std = 0.03;
        s.boat_std = 0.0005;
        s.manual_std = 0.02;
        s.alt_std = 0.10;
        return s;
    }
    
    static StandardDeviationSettings loose_settings() {
        StandardDeviationSettings s;
        s.normal_std = 0.08;
        s.boat_std = 0.002;
        s.manual_std = 0.05;
        s.alt_std = 0.20;
        return s;
    }
};

struct CalculatorSettings {
    int number_of_predictions = 5;
    bool use_advanced_statistics = true;
    core::McVersion mc_version = core::McVersion::V1_19_plus;
    StandardDeviationSettings std_dev_settings;
    double mismeasure_threshold = 3.0;
    bool enable_mismeasure_warnings = true;
    bool enable_parallel_throw_warnings = true;
    
    static CalculatorSettings default_settings() {
        return CalculatorSettings{};
    }
};

}
