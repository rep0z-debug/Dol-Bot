#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include "dolbot/core/coords.hpp"
#include "dolbot/domain/fortress_ring.hpp"

namespace dolbot::domain {

class ApproxDensity {
public:
    static ApproxDensity& instance() {
        static ApproxDensity inst;
        return inst;
    }
    
    double get_density(double chunk_r) const {
        double k = chunk_r / delta_r_;
        double t = k - static_cast<int>(k);
        int i0 = static_cast<int>(k);
        int i1 = i0 + 1;
        
        if (i1 >= static_cast<int>(density_table_.size())) {
            return i0 < static_cast<int>(density_table_.size()) ? density_table_[i0] : 0.0;
        }
        
        return (1.0 - t) * density_table_[i0] + t * density_table_[i1];
    }
    
    double get_density_cartesian(double cx, double cz) const {
        double d2 = cx * cx + cz * cz;
        return get_density(std::sqrt(d2));
    }
    
    double get_cumulative(double chunk_r) const {
        if (chunk_r < 0.0) return 0.0;
        
        double k = chunk_r / delta_r_;
        double t = k - static_cast<int>(k);
        int i0 = static_cast<int>(k);
        int i1 = i0 + 1;
        
        if (i1 >= static_cast<int>(cumulative_table_.size())) {
            return cumulative_table_.back();
        }
        
        return (1.0 - t) * cumulative_table_[i0] + t * cumulative_table_[i1];
    }
    
    double cumulative_polar(double r) const {
        return get_cumulative(r);
    }

private:
    static constexpr int MAX_CHUNK = 2000;
    static constexpr double delta_r_ = 0.5;
    static constexpr double SMOOTHING_SIGMA = 4.5;
    static constexpr int KERNEL_HALF_SIZE = 18;
    
    ApproxDensity() {
        compute_density_table();
        apply_gaussian_smoothing();
        compute_cumulative_table();
    }
    
    void compute_density_table() {
        int table_size = static_cast<int>(MAX_CHUNK / delta_r_) + 5;
        density_table_.resize(table_size, 0.0);
        
        const auto& ring_sys = RingSystem::instance();
        
        for (const auto& ring : ring_sys.all()) {
            double r0 = ring.inner_radius;
            double r1 = ring.outer_radius;
            
            for (double r = r0; r <= r1; r += delta_r_) {
                if (r < 1.0) continue;
                
                double rho = ring.stronghold_count / 
                    (2.0 * core::coords::PI * (ring.outer_radius - ring.inner_radius) * r);
                
                if (std::abs(r - r0) < delta_r_ || std::abs(r - r1) < delta_r_) {
                    rho *= 0.5;
                }
                
                int idx = static_cast<int>(r / delta_r_);
                if (idx >= 0 && idx < static_cast<int>(density_table_.size())) {
                    density_table_[idx] = rho;
                }
            }
        }
    }
    
    void apply_gaussian_smoothing() {
        std::vector<double> kernel = compute_gaussian_kernel();
        
        std::vector<double> density_pre = density_table_;
        std::fill(density_table_.begin(), density_table_.end(), 0.0);
        
        for (size_t i = 0; i < density_table_.size(); ++i) {
            double weighted_sum = 0.0;
            double weight_total = 0.0;
            
            for (int j = -KERNEL_HALF_SIZE; j <= KERNEL_HALF_SIZE; ++j) {
                int idx = static_cast<int>(i) + j;
                if (idx >= 0 && idx < static_cast<int>(density_pre.size())) {
                    double w = kernel[std::abs(j)];
                    weighted_sum += density_pre[idx] * w;
                    weight_total += w;
                }
            }
            
            if (weight_total > 0.0) {
                density_table_[i] = weighted_sum / weight_total;
            }
        }
    }
    
    std::vector<double> compute_gaussian_kernel() const {
        std::vector<double> kernel(KERNEL_HALF_SIZE + 1);
        double sum = 0.0;
        
        for (int i = 0; i <= KERNEL_HALF_SIZE; ++i) {
            double x = static_cast<double>(i);
            kernel[i] = std::exp(-0.5 * (x * x) / (SMOOTHING_SIGMA * SMOOTHING_SIGMA));
            sum += (i == 0) ? kernel[i] : 2.0 * kernel[i];
        }
        
        for (auto& k : kernel) {
            k /= sum;
        }
        
        return kernel;
    }
    
    void compute_cumulative_table() {
        cumulative_table_.resize(density_table_.size(), 0.0);
        double cumsum = 0.0;
        
        for (size_t i = 0; i < cumulative_table_.size(); ++i) {
            cumsum += density_table_[i] * i * delta_r_ * 2.0 * core::coords::PI;
            cumulative_table_[i] = cumsum;
        }
    }
    
    std::vector<double> density_table_;
    std::vector<double> cumulative_table_;
};

}
