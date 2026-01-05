#pragma once

#include <cmath>
#include <vector>
#include "dolbot/core/coords.hpp"
#include "dolbot/domain/fortress_ring.hpp"
#include "dolbot/domain/approx_density.hpp"

namespace dolbot::domain {

class ClosestStrongholdConditioner {
public:
    static constexpr double PROBABILITY_THRESHOLD = 0.001;
    static constexpr int ANGULAR_SAMPLES = 32;
    static constexpr int RADIAL_SAMPLES = 5;
    static constexpr double CLOSE_RANGE_THRESHOLD = 2000.0;
    static constexpr double FAR_RANGE_THRESHOLD = 3000.0;
    
    struct ChunkData {
        int x;
        int z;
        double weight;
    };
    
    static double condition_chunk(
        const ChunkData& chunk,
        double player_x,
        double player_z,
        core::McVersion version
    ) {
        int stronghold_offset = get_stronghold_chunk_coord(version);
        double sh_x = chunk.x * 16.0 + stronghold_offset;
        double sh_z = chunk.z * 16.0 + stronghold_offset;
        
        double dist_to_candidate = std::sqrt(
            (sh_x - player_x) * (sh_x - player_x) + 
            (sh_z - player_z) * (sh_z - player_z)
        );
        
        double chunk_r = std::sqrt(static_cast<double>(chunk.x * chunk.x + chunk.z * chunk.z));
        const FortressRing* candidate_ring = RingSystem::instance().find_ring(chunk_r);
        
        if (!candidate_ring) return 0.0;
        
        double player_dist = std::sqrt(player_x * player_x + player_z * player_z);
        
        double closest_probability = 1.0;
        
        for (int ring_idx = 0; ring_idx < NUM_RINGS; ++ring_idx) {
            const auto& ring = RingSystem::instance().get(ring_idx);
            
            double prob_closer = compute_probability_ring_has_closer_adaptive(
                ring, dist_to_candidate, player_x, player_z, 
                candidate_ring->ring_index == ring_idx,
                player_dist
            );
            
            closest_probability *= (1.0 - prob_closer);
        }
        
        return std::max(0.0, std::min(1.0, closest_probability));
    }

private:
    static double compute_probability_ring_has_closer_adaptive(
        const FortressRing& ring,
        double dist_to_candidate,
        double player_x,
        double player_z,
        bool is_same_ring,
        double player_dist
    ) {
        double player_r = std::sqrt(player_x * player_x + player_z * player_z) / 16.0;
        double player_phi = core::coords::phi_from_coords(player_x, player_z);
        
        double ring_r_min = ring.inner_radius_snapped;
        double ring_r_max = ring.outer_radius_snapped;
        
        double min_possible_dist = std::abs(player_r - ring_r_min) * 16.0;
        
        if (min_possible_dist >= dist_to_candidate) {
            return 0.0;
        }
        
        int num_strongholds_in_ring = is_same_ring ? (ring.stronghold_count - 1) : ring.stronghold_count;
        
        if (num_strongholds_in_ring <= 0) return 0.0;
        
        double angular_span = 2.0 * core::coords::PI / ring.stronghold_count;
        
        double prob_single;
        
        if (player_dist < CLOSE_RANGE_THRESHOLD) {
            prob_single = compute_single_stronghold_closer_prob_fast(
                ring_r_min, ring_r_max, angular_span,
                player_r, player_phi,
                dist_to_candidate / 16.0
            );
        } else if (player_dist > FAR_RANGE_THRESHOLD) {
            prob_single = compute_single_stronghold_closer_prob_enhanced(
                ring_r_min, ring_r_max, angular_span,
                player_r, player_phi,
                dist_to_candidate / 16.0
            );
        } else {
            double t = (player_dist - CLOSE_RANGE_THRESHOLD) / (FAR_RANGE_THRESHOLD - CLOSE_RANGE_THRESHOLD);
            double prob_fast = compute_single_stronghold_closer_prob_fast(
                ring_r_min, ring_r_max, angular_span,
                player_r, player_phi,
                dist_to_candidate / 16.0
            );
            double prob_enhanced = compute_single_stronghold_closer_prob_enhanced(
                ring_r_min, ring_r_max, angular_span,
                player_r, player_phi,
                dist_to_candidate / 16.0
            );
            prob_single = (1.0 - t) * prob_fast + t * prob_enhanced;
        }
        
        return 1.0 - std::pow(1.0 - prob_single, num_strongholds_in_ring);
    }
    
    static double compute_single_stronghold_closer_prob_fast(
        double r_min,
        double r_max,
        double angular_span,
        double player_r,
        double player_phi,
        double dist_threshold_chunks
    ) {
        double ring_center = (r_min + r_max) / 2.0;
        double ring_thickness = r_max - r_min;
        
        double px = -player_r * std::sin(player_phi);
        double pz = player_r * std::cos(player_phi);
        
        double min_dist_to_ring = std::abs(std::sqrt(px * px + pz * pz) - ring_center);
        double max_dist_to_ring = std::sqrt(px * px + pz * pz) + ring_center;
        
        if (min_dist_to_ring >= dist_threshold_chunks) {
            return 0.0;
        }
        
        if (max_dist_to_ring <= dist_threshold_chunks) {
            return angular_span / (2.0 * core::coords::PI);
        }
        
        double coverage_radial = std::min(1.0, 
            (dist_threshold_chunks - min_dist_to_ring) / ring_thickness);
        
        double angular_coverage = 0.0;
        double dist_sq_threshold = dist_threshold_chunks * dist_threshold_chunks;
        
        for (int ai = 0; ai < 8; ++ai) {
            double phi = 2.0 * core::coords::PI * ai / 8.0;
            double sh_x = -ring_center * std::sin(phi);
            double sh_z = ring_center * std::cos(phi);
            double dist_sq = (sh_x - px) * (sh_x - px) + (sh_z - pz) * (sh_z - pz);
            if (dist_sq < dist_sq_threshold) {
                angular_coverage += 1.0;
            }
        }
        angular_coverage /= 8.0;
        
        double prob = coverage_radial * angular_coverage;
        
        return prob * angular_span / (2.0 * core::coords::PI);
    }
    
    static double compute_single_stronghold_closer_prob_enhanced(
        double r_min,
        double r_max,
        double angular_span,
        double player_r,
        double player_phi,
        double dist_threshold_chunks
    ) {
        double closer_weight = 0.0;
        double total_weight = 0.0;
        
        double px = -player_r * std::sin(player_phi);
        double pz = player_r * std::cos(player_phi);
        
        for (int ri = 0; ri < RADIAL_SAMPLES; ++ri) {
            double r = r_min + (r_max - r_min) * (ri + 0.5) / RADIAL_SAMPLES;
            
            double radial_weight = ApproxDensity::instance().get_density(r) * r;
            
            for (int ai = 0; ai < ANGULAR_SAMPLES; ++ai) {
                double phi = 2.0 * core::coords::PI * ai / ANGULAR_SAMPLES;
                
                double sh_x = -r * std::sin(phi);
                double sh_z = r * std::cos(phi);
                
                double dist = std::sqrt((sh_x - px) * (sh_x - px) + (sh_z - pz) * (sh_z - pz));
                
                double weight = radial_weight;
                total_weight += weight;
                
                if (dist < dist_threshold_chunks) {
                    closer_weight += weight;
                }
            }
        }
        
        if (total_weight <= 0.0) return 0.0;
        
        double base_prob = closer_weight / total_weight;
        
        return base_prob * angular_span / (2.0 * core::coords::PI);
    }
};

}
