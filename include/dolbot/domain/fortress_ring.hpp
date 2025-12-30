#pragma once

#include <vector>
#include <cmath>
#include <utility>
#include <stdexcept>
#include "dolbot/core/coords.hpp"
#include "dolbot/core/config.hpp"

namespace dolbot::domain {

constexpr int NUM_RINGS = 8;
constexpr int DIST_PARAM = 32;
constexpr int SNAPPING_RADIUS = 7;
constexpr int TOTAL_STRONGHOLDS = 128;

class FortressRing {
public:
    int ring_index;
    int stronghold_count;
    double inner_radius;
    double outer_radius;
    double inner_radius_snapped;
    double outer_radius_snapped;
    
    FortressRing(int index, int count)
        : ring_index(index)
        , stronghold_count(count)
    {
        double base = DIST_PARAM * (4 + index * 6);
        inner_radius = base - 0.5 * 2.5 * DIST_PARAM;
        outer_radius = base + 0.5 * 2.5 * DIST_PARAM;
        
        double snap_offset = (SNAPPING_RADIUS + 1.0) * std::sqrt(2.0);
        inner_radius_snapped = inner_radius - snap_offset;
        outer_radius_snapped = outer_radius + snap_offset;
    }
    
    [[nodiscard]] double center_radius() const {
        return (inner_radius + outer_radius) / 2.0;
    }
    
    [[nodiscard]] bool contains_radius(double chunk_r) const {
        return chunk_r >= inner_radius_snapped && chunk_r <= outer_radius_snapped;
    }
    
    [[nodiscard]] double section_angle() const {
        return 2.0 * core::coords::PI / stronghold_count;
    }
};

class RingSystem {
public:
    static const RingSystem& instance() {
        static RingSystem inst;
        return inst;
    }
    
    const FortressRing& get(int index) const {
        if (index < 0 || index >= NUM_RINGS) {
            throw std::out_of_range("Invalid ring index");
        }
        return rings_[index];
    }
    
    const FortressRing* find_ring(double chunk_r) const {
        for (const auto& ring : rings_) {
            if (ring.contains_radius(chunk_r)) {
                return &ring;
            }
        }
        return nullptr;
    }
    
    std::pair<const FortressRing*, const FortressRing*> find_closest_rings(double cx, double cz) const {
        double r = std::sqrt(cx * cx + cz * cz);
        
        const FortressRing* closest = &rings_[0];
        const FortressRing* next_closest = &rings_[1];
        double closest_dist = std::abs(closest->center_radius() - r);
        double next_dist = std::abs(next_closest->center_radius() - r);
        
        if (next_dist < closest_dist) {
            std::swap(closest, next_closest);
            std::swap(closest_dist, next_dist);
        }
        
        for (int i = 2; i < NUM_RINGS; ++i) {
            double dist = std::abs(rings_[i].center_radius() - r);
            if (dist < closest_dist) {
                next_closest = closest;
                next_dist = closest_dist;
                closest = &rings_[i];
                closest_dist = dist;
            } else if (dist < next_dist) {
                next_closest = &rings_[i];
                next_dist = dist;
            }
        }
        
        return {closest, next_closest};
    }
    
    const std::vector<FortressRing>& all() const { return rings_; }

private:
    RingSystem() {
        rings_.reserve(NUM_RINGS);
        int counts[] = {3, 6, 10, 15, 21, 28, 36, 9};
        for (int i = 0; i < NUM_RINGS; ++i) {
            rings_.emplace_back(i, counts[i]);
        }
    }
    
    std::vector<FortressRing> rings_;
};

inline int get_stronghold_chunk_coord(core::McVersion version) {
    switch (version) {
        case core::McVersion::Pre_1_9:
        case core::McVersion::V1_9_to_1_12:
        case core::McVersion::V1_13_to_1_18:
            return 8;
        case core::McVersion::V1_19_plus:
        default:
            return 0;
    }
}

} // namespace dolbot::domain
