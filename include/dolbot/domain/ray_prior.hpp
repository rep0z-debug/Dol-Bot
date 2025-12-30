#pragma once

#include <vector>
#include <cmath>
#include <utility>
#include "dolbot/domain/world_chunk.hpp"
#include "dolbot/domain/fortress_ring.hpp"
#include "dolbot/domain/divine_context.hpp"
#include "dolbot/domain/approx_density.hpp"

namespace dolbot::domain {

class RayApproximatedPrior {
public:
    RayApproximatedPrior(
        double player_x, 
        double player_z, 
        double horizontal_angle,
        double tolerance = 1.0,
        core::McVersion version = core::McVersion::V1_19_plus,
        const DivineContext& divine_context = DivineContext()
    )
        : player_x_(player_x)
        , player_z_(player_z)
        , angle_(horizontal_angle)
        , tolerance_deg_(tolerance)
        , version_(version)
        , divine_context_(divine_context)
    {
        generate_candidates();
    }
    
    const ChunkList& candidates() const { return chunks_; }

private:
    static constexpr double MAX_RANGE_CHUNKS = 350.0;
    static constexpr int DISCRETIZATION_SAMPLES = 4;
    
    void generate_candidates() {
        chunks_.clear();
        
        double center_rad = angle_ * core::coords::PI / 180.0;
        double tol_rad = tolerance_deg_ * core::coords::PI / 180.0;
        
        double dir_x = -std::sin(center_rad);
        double dir_z = std::cos(center_rad);
        
        int sh_offset = get_stronghold_chunk_coord(version_);
        double origin_chunk_x = (player_x_ - sh_offset) / 16.0;
        double origin_chunk_z = (player_z_ - sh_offset) / 16.0;
        
        const auto& ring_sys = RingSystem::instance();
        
        for (const auto& ring : ring_sys.all()) {
            auto intersections = intersect_ray_with_annulus(
                origin_chunk_x, origin_chunk_z,
                dir_x, dir_z,
                ring.inner_radius_snapped, ring.outer_radius_snapped
            );
            
            if (!intersections.first && !intersections.second) continue;
            
            double t_near = intersections.first.value_or(0.0);
            double t_far = intersections.second.value_or(MAX_RANGE_CHUNKS);
            
            if (t_near < 0) t_near = 0;
            if (t_far > MAX_RANGE_CHUNKS) t_far = MAX_RANGE_CHUNKS;
            if (t_near > t_far) continue;
            
            collect_chunks_along_ray(
                origin_chunk_x, origin_chunk_z,
                center_rad, tol_rad,
                t_near, t_far, ring
            );
        }
    }
    
    std::pair<std::optional<double>, std::optional<double>> intersect_ray_with_annulus(
        double ox, double oz,
        double dx, double dz,
        double r_inner, double r_outer
    ) {
        auto outer_hits = intersect_ray_circle(ox, oz, dx, dz, r_outer);
        auto inner_hits = intersect_ray_circle(ox, oz, dx, dz, r_inner);
        
        std::optional<double> t_near, t_far;
        
        double origin_r = std::sqrt(ox * ox + oz * oz);
        
        if (origin_r >= r_inner && origin_r <= r_outer) {
            t_near = 0.0;
            if (outer_hits.first) {
                t_far = *outer_hits.first > 0 ? outer_hits.first : outer_hits.second;
            }
        } else if (origin_r < r_inner) {
            if (inner_hits.first && *inner_hits.first > 0) {
                t_near = inner_hits.first;
                if (outer_hits.first && *outer_hits.first > 0) {
                    t_far = std::min(*outer_hits.first, *inner_hits.second.value_or(*outer_hits.first));
                }
            } else if (outer_hits.first && *outer_hits.first > 0) {
                t_near = outer_hits.first;
                t_far = outer_hits.second;
            }
        } else {
            if (outer_hits.first && *outer_hits.first > 0) {
                t_near = outer_hits.first;
                if (inner_hits.first) {
                    t_far = inner_hits.first;
                } else {
                    t_far = outer_hits.second;
                }
            }
        }
        
        return {t_near, t_far};
    }
    
    std::pair<std::optional<double>, std::optional<double>> intersect_ray_circle(
        double ox, double oz,
        double dx, double dz,
        double r
    ) {
        double a = dx * dx + dz * dz;
        double b = 2.0 * (ox * dx + oz * dz);
        double c = ox * ox + oz * oz - r * r;
        
        double discriminant = b * b - 4.0 * a * c;
        
        if (discriminant < 0 || a < 1e-10) {
            return {std::nullopt, std::nullopt};
        }
        
        double sqrt_disc = std::sqrt(discriminant);
        double t1 = (-b - sqrt_disc) / (2.0 * a);
        double t2 = (-b + sqrt_disc) / (2.0 * a);
        
        return {t1, t2};
    }
    
    void collect_chunks_along_ray(
        double ox, double oz,
        double center_rad, double tol_rad,
        double t_start, double t_end,
        const FortressRing& ring
    ) {
        double dir_x = -std::sin(center_rad);
        double dir_z = std::cos(center_rad);
        
        double step = 1.0;
        
        for (double t = t_start; t <= t_end; t += step) {
            double cx = ox + dir_x * t;
            double cz = oz + dir_z * t;
            
            int chunk_x = static_cast<int>(std::floor(cx));
            int chunk_z = static_cast<int>(std::floor(cz));
            
            double tan_tol = std::tan(tol_rad);
            int lateral_range = static_cast<int>(std::ceil(t * tan_tol)) + 1;
            
            double perp_x = dir_z;
            double perp_z = -dir_x;
            
            for (int lat = -lateral_range; lat <= lateral_range; ++lat) {
                int cx_adj = chunk_x + static_cast<int>(std::round(perp_x * lat));
                int cz_adj = chunk_z + static_cast<int>(std::round(perp_z * lat));
                
                if (std::abs(cx_adj) > MAX_RANGE_CHUNKS || std::abs(cz_adj) > MAX_RANGE_CHUNKS) continue;
                
                double to_chunk_x = cx_adj - ox;
                double to_chunk_z = cz_adj - oz;
                double chunk_angle = std::atan2(-to_chunk_x, to_chunk_z);
                double angle_diff = std::abs(chunk_angle - center_rad);
                while (angle_diff > core::coords::PI) angle_diff -= 2.0 * core::coords::PI;
                angle_diff = std::abs(angle_diff);
                
                if (angle_diff <= tol_rad * 1.5) {
                    double chunk_r = std::sqrt(static_cast<double>(cx_adj * cx_adj + cz_adj * cz_adj));
                    if (chunk_r >= ring.inner_radius_snapped && chunk_r <= ring.outer_radius_snapped) {
                        add_chunk_if_new(cx_adj, cz_adj);
                    }
                }
            }
        }
    }
    
    void add_chunk_if_new(int cx, int cz) {
        for (const auto& existing : chunks_) {
            if (existing.pos.x == cx && existing.pos.z == cz) return;
        }
        
        double weight = compute_chunk_weight(static_cast<double>(cx), static_cast<double>(cz));
        if (weight > 0.0) {
            chunks_.emplace_back(cx, cz, weight);
        }
    }
    
    double compute_chunk_weight(double cx, double cz) {
        double base_density = ApproxDensity::instance().get_density_cartesian(cx, cz);
        
        if (base_density <= 0.0) return 0.0;
        
        double angular_factor = 1.0;
        
        double chunk_r = std::sqrt(cx * cx + cz * cz);
        const auto& ring_sys = RingSystem::instance();
        
        if (chunk_r <= ring_sys.get(0).outer_radius_snapped && divine_context_.has_divine()) {
            double phi = core::coords::phi_from_coords(cx, cz);
            double w = divine_context_.density_at_angle_before_snapping(phi);
            angular_factor = w * 2.0 * core::coords::PI;
        }
        
        return base_density * angular_factor;
    }
    
    double player_x_;
    double player_z_;
    double angle_;
    double tolerance_deg_;
    core::McVersion version_;
    DivineContext divine_context_;
    ChunkList chunks_;
};

}
