#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include "dolbot/domain/world_chunk.hpp"
#include "dolbot/domain/fortress_ring.hpp"
#include "dolbot/domain/divine_context.hpp"
#include "dolbot/domain/approx_density.hpp"
#include "dolbot/domain/eye_throw.hpp"

namespace dolbot::domain {

class ProbabilityPrior {
public:
    ProbabilityPrior(const EyeThrow& ray_throw, double tolerance_rad, 
                     core::McVersion version,
                     const DivineContext& divine_context = DivineContext())
        : divine_context_(divine_context)
        , version_(version)
    {
        generate_from_ray(ray_throw, tolerance_rad);
    }

    ProbabilityPrior(int center_chunk_x, int center_chunk_z, int search_radius_chunks, 
                     bool use_advanced_stats = true,
                     const DivineContext& divine_context = DivineContext())
        : center_x_(center_chunk_x)
        , center_z_(center_chunk_z)
        , radius_(search_radius_chunks)
        , divine_context_(divine_context)
    {
        generate_grid();
        if (use_advanced_stats) {
            apply_gaussian_smoothing();
        }
    }
    
    ProbabilityPrior(const core::Vec2d& center_blocks, int search_radius_chunks, 
                     bool use_advanced_stats = true,
                     const DivineContext& divine_context = DivineContext())
        : ProbabilityPrior(
            static_cast<int>(std::floor(center_blocks.x / 16.0)), 
            static_cast<int>(std::floor(center_blocks.z / 16.0)), 
            search_radius_chunks, 
            use_advanced_stats,
            divine_context
        )
    {}
    
    const ChunkList& candidates() const { return chunks_; }
    
    double total_weight() const {
        double sum = 0.0;
        for (const auto& c : chunks_) {
            sum += c.probability;
        }
        return sum;
    }
    
    void normalize() {
        double total = total_weight();
        if (total <= 0.0) return;
        for (auto& c : chunks_) {
            c.probability /= total;
        }
    }

private:
    static constexpr double MAX_RANGE_CHUNKS = 350.0;
    static constexpr double SMOOTH_SIGMA = 4.0;
    static constexpr int SMOOTH_RADIUS = 10;
    
    void generate_from_ray(const EyeThrow& throw_ray, double tolerance_rad) {
        chunks_.clear();
        
        double angle_rad = throw_ray.corrected_angle() * core::coords::DEG_TO_RAD;
        double tolerance_deg = tolerance_rad * 180.0 / core::coords::PI;
        
        double dir_x = -std::sin(angle_rad);
        double dir_z = std::cos(angle_rad);
        
        int sh_offset = get_stronghold_chunk_coord(version_);
        double origin_x = (throw_ray.x_overworld() - sh_offset) / 16.0;
        double origin_z = (throw_ray.z_overworld() - sh_offset) / 16.0;
        
        const auto& ring_sys = RingSystem::instance();
        
        for (const auto& ring : ring_sys.all()) {
            collect_chunks_in_cone(
                origin_x, origin_z,
                angle_rad, tolerance_rad,
                ring.inner_radius_snapped, ring.outer_radius_snapped
            );
        }
    }
    
    void collect_chunks_in_cone(
        double ox, double oz,
        double center_rad, double tol_rad,
        double r_inner, double r_outer
    ) {
        double cos_tol = std::cos(tol_rad);
        double dir_x = -std::sin(center_rad);
        double dir_z = std::cos(center_rad);
        
        int r_max = static_cast<int>(std::min(r_outer, MAX_RANGE_CHUNKS)) + 1;
        int r_min = std::max(0, static_cast<int>(r_inner) - 1);
        
        for (int cx = -r_max; cx <= r_max; ++cx) {
            for (int cz = -r_max; cz <= r_max; ++cz) {
                double chunk_r = std::sqrt(static_cast<double>(cx * cx + cz * cz));
                
                if (chunk_r < r_inner || chunk_r > r_outer) continue;
                if (chunk_r > MAX_RANGE_CHUNKS) continue;
                
                double to_chunk_x = cx - ox;
                double to_chunk_z = cz - oz;
                double dist = std::sqrt(to_chunk_x * to_chunk_x + to_chunk_z * to_chunk_z);
                
                if (dist < 0.1) continue;
                
                double dot = (to_chunk_x * dir_x + to_chunk_z * dir_z) / dist;
                
                if (dot >= cos_tol) {
                    add_chunk_if_new(cx, cz);
                }
            }
        }
    }
    
    void add_chunk_if_new(int cx, int cz) {
        for (const auto& existing : chunks_) {
            if (existing.pos.x == cx && existing.pos.z == cz) return;
        }
        
        double weight = compute_chunk_weight(cx, cz);
        if (weight > 0.0) {
            chunks_.emplace_back(cx, cz, weight);
        }
    }
    
    double compute_chunk_weight(int cx, int cz) {
        double base_density = ApproxDensity::instance().get_density_cartesian(
            static_cast<double>(cx), static_cast<double>(cz)
        );
        
        if (base_density <= 0.0) return 0.0;
        
        double angular_factor = 1.0;
        
        double chunk_r = std::sqrt(static_cast<double>(cx * cx + cz * cz));
        const auto& ring_sys = RingSystem::instance();
        
        if (chunk_r <= ring_sys.get(0).outer_radius_snapped && divine_context_.has_divine()) {
            double phi = core::coords::phi_from_coords(
                static_cast<double>(cx), static_cast<double>(cz)
            );
            double w = divine_context_.density_at_angle_before_snapping(phi);
            angular_factor = w * 2.0 * core::coords::PI;
        }
        
        return base_density * angular_factor;
    }

    void generate_grid() {
        chunks_.clear();
        
        int search_r = radius_ + SNAPPING_RADIUS;
        
        for (int dx = -search_r; dx <= search_r; ++dx) {
            for (int dz = -search_r; dz <= search_r; ++dz) {
                int cx = center_x_ + dx;
                int cz = center_z_ + dz;
                
                double chunk_r = std::sqrt(static_cast<double>(cx * cx + cz * cz));
                const FortressRing* ring = RingSystem::instance().find_ring(chunk_r);
                
                if (!ring) continue;
                
                double weight = compute_chunk_weight(cx, cz);
                if (weight > 0.0) {
                    chunks_.emplace_back(cx, cz, weight);
                }
            }
        }
    }
    
    void apply_gaussian_smoothing() {
        if (chunks_.empty()) return;
        
        std::vector<double> kernel = compute_gaussian_kernel_1d();
        
        std::map<std::pair<int, int>, double> old_weights;
        for (const auto& c : chunks_) {
            old_weights[{c.pos.x, c.pos.z}] = c.probability;
        }
        
        for (auto& c : chunks_) {
            double weighted_sum = 0.0;
            double weight_total = 0.0;
            
            for (int kx = -SMOOTH_RADIUS; kx <= SMOOTH_RADIUS; ++kx) {
                for (int kz = -SMOOTH_RADIUS; kz <= SMOOTH_RADIUS; ++kz) {
                    auto it = old_weights.find({c.pos.x + kx, c.pos.z + kz});
                    if (it != old_weights.end()) {
                        double w = kernel[std::abs(kx)] * kernel[std::abs(kz)];
                        weighted_sum += it->second * w;
                        weight_total += w;
                    }
                }
            }
            
            c.probability = weight_total > 0.0 ? weighted_sum / weight_total : 0.0;
        }
    }
    
    std::vector<double> compute_gaussian_kernel_1d() const {
        std::vector<double> kernel(SMOOTH_RADIUS + 1);
        double sum = 0.0;
        
        for (int i = 0; i <= SMOOTH_RADIUS; ++i) {
            double x = static_cast<double>(i);
            kernel[i] = std::exp(-0.5 * x * x / (SMOOTH_SIGMA * SMOOTH_SIGMA));
            sum += (i == 0) ? kernel[i] : 2.0 * kernel[i];
        }
        
        for (auto& k : kernel) {
            k /= sum;
        }
        
        return kernel;
    }
    
    int center_x_ = 0;
    int center_z_ = 0;
    int radius_ = 0;
    DivineContext divine_context_;
    core::McVersion version_ = core::McVersion::V1_19_plus;
    ChunkList chunks_;
    
    std::map<std::pair<int, int>, double> chunk_map_;
};

}
