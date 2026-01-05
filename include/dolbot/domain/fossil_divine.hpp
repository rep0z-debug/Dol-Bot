#pragma once

#include <cmath>
#include <optional>
#include <array>
#include <string>
#include <regex>
#include <algorithm>
#include "dolbot/core/coords.hpp"
#include "dolbot/domain/fortress_ring.hpp"

namespace dolbot::domain {

struct FossilLocation {
    double x;
    double z;
    int segment; 
};

struct DivineResult {
    double direction_angle = 0.0;
    double opposite_angle = 0.0;
    int ring_index = 0;
    
    [[nodiscard]] bool is_valid() const {
        return ring_index >= 0 && ring_index <= 7;
    }
};

class FossilDivine {
public:
    static constexpr int SEGMENTS = 16;
    
    static std::optional<FossilLocation> parse_f3i_bone(const std::string& clipboard_text) {
        static const std::regex setblock_pattern(
            R"(^/setblock\s+(-?\d+)\s+(?:-?\d+)\s+(-?\d+)\s+minecraft:bone_block\[axis=(x|z)\])"
        );
        
        std::smatch match;
        if (std::regex_search(clipboard_text, match, setblock_pattern)) {
            double x = std::stod(match[1]);
            double z = std::stod(match[2]);
            std::string axis = match[3];
            
            int segment = segment_from_position(x, z); 
            return FossilLocation{x, z, segment};
        }
        return std::nullopt;
    }

    static std::optional<DivineResult> compute(const FossilLocation& fossil) {
        if (fossil.segment < 0 || fossil.segment >= SEGMENTS) {
            return std::nullopt;
        }
        
        DivineResult result;
        
        double segment_angle = 360.0 / SEGMENTS;
        double base_angle = fossil.segment * segment_angle;
        result.direction_angle = core::coords::normalize_angle(base_angle + segment_angle / 2.0);
        result.opposite_angle = core::coords::normalize_angle(result.direction_angle + 180.0);
        
        double dist = std::sqrt(fossil.x * fossil.x + fossil.z * fossil.z);
        result.ring_index = estimate_ring_from_distance(dist);
        
        return result;
    }
    
    static int segment_from_position(double x, double z) {
        double angle = std::atan2(x, -z); 
        
        if (angle < 0) angle += 2.0 * core::coords::PI;
        
        double segment_angle = 2.0 * core::coords::PI / SEGMENTS;
        int segment = static_cast<int>(angle / segment_angle);
        
        return std::clamp(segment, 0, SEGMENTS - 1);
    }
    
    static std::array<double, 2> get_segment_angle_range(int segment) {
        if (segment < 0 || segment >= SEGMENTS) {
            return {0.0, 0.0};
        }
        
        double segment_angle = 360.0 / SEGMENTS;
        double start = segment * segment_angle;
        double end = start + segment_angle;
        
        return {start, end};
    }
    
    static core::Vec2d get_divine_coords(const FossilLocation& fossil, double target_distance) {
        auto result = compute(fossil);
        if (!result) return {0.0, 0.0};
        
        double rad = core::coords::to_radians(result->direction_angle);
        // Corrected math:
        // North (0 rad) -> x=0, z=-dist
        // East (PI/2 rad) -> x=dist, z=0
        // South (PI rad) -> x=0, z=dist
        // West (-PI/2 rad) -> x=-dist, z=0
        return {
            std::sin(rad) * target_distance,
            -std::cos(rad) * target_distance
        };
    }

private:
    static int estimate_ring_from_distance(double distance) {
        const auto& rings = RingSystem::instance().all();
        
        for (size_t i = 0; i < rings.size(); ++i) {
            if (distance < rings[i].outer_radius * 16) {
                return static_cast<int>(i);
            }
        }
        
        return 0;
    }
};

} // namespace dolbot::domain
