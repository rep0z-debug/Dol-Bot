#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <map>
#include "dolbot/domain/world_chunk.hpp"
#include "dolbot/domain/eye_throw.hpp"
#include "dolbot/domain/probability_prior.hpp"
#include "dolbot/domain/divine_context.hpp"
#include "dolbot/domain/closest_stronghold.hpp"

namespace dolbot::domain {

struct PredictionResult {
    WorldChunk chunk;
    double certainty = 0.0;
    std::vector<double> angle_errors;
    
    bool likely_mismeasure = false;
    bool poor_triangulation = false;
    std::string warning_message;
    std::optional<double> distance;
    std::optional<core::Vec2d> last_throw_pos;
    
    [[nodiscard]] double average_error() const {
        if (angle_errors.empty()) return 0.0;
        double sum = 0.0;
        for (double e : angle_errors) sum += std::abs(e);
        return sum / angle_errors.size();
    }
    
    [[nodiscard]] double max_error() const {
        if (angle_errors.empty()) return 0.0;
        double max_e = 0.0;
        for (double e : angle_errors) max_e = std::max(max_e, std::abs(e));
        return max_e;
    }
    
    [[nodiscard]] double rms_error() const {
        if (angle_errors.empty()) return 0.0;
        double sum_sq = 0.0;
        for (double e : angle_errors) sum_sq += e * e;
        return std::sqrt(sum_sq / angle_errors.size());
    }
    
    [[nodiscard]] bool has_warning() const {
        return likely_mismeasure || poor_triangulation;
    }
};


class ProbabilityPosterior {
public:
    ProbabilityPosterior(const std::vector<EyeThrow>& throws, 
                         double std_deviation,
                         double std_dev_boat,
                         double std_dev_manual,
                         bool use_advanced_stats,
                         core::McVersion version,
                         const DivineContext& divine_context = DivineContext())
        : throws_(throws)
        , std_dev_(std_deviation)
        , std_dev_boat_(std_dev_boat)
        , std_dev_manual_(std_dev_manual)
        , use_advanced_(use_advanced_stats)
        , version_(version)
        , divine_context_(divine_context)
    {
        if (!throws_.empty()) {
            compute_posterior();
        }
    }
    
    ProbabilityPosterior(const std::vector<EyeThrow>& throws, 
                         double std_deviation,
                         bool use_advanced_stats,
                         core::McVersion version)
        : ProbabilityPosterior(throws, std_deviation, 0.001, std_deviation, use_advanced_stats, version)
    {}
    
    [[nodiscard]] const ChunkList& ranked_chunks() const { return ranked_; }
    
    [[nodiscard]] std::vector<PredictionResult> top_predictions(int count = 5, double mismeasure_threshold = 3.0, std::optional<core::Vec2d> player_pos = std::nullopt) const {
        std::vector<PredictionResult> results;
        int n = std::min(count, static_cast<int>(ranked_.size()));
        
        double best_cross = compute_best_cross_product();
        bool is_poor_triangulation = (throws_.size() >= 2 && best_cross < 0.02);
        
        double concentration = compute_probability_concentration();
        double base_confidence = compute_base_confidence(concentration);
                
        for (int i = 0; i < n; ++i) {
            PredictionResult pred;
            pred.chunk = ranked_[i];
            
            double relative_weight = (i == 0) ? 1.0 : 
                (ranked_[0].probability > 0 ? ranked_[i].probability / ranked_[0].probability : 0.0);
            pred.certainty = base_confidence * relative_weight;
            
            if (player_pos.has_value()) {
                 int sh_x = pred.chunk.stronghold_x(version_);
                 int sh_z = pred.chunk.stronghold_z(version_);
                 double dx = sh_x - player_pos->x;
                 double dz = sh_z - player_pos->z;
                 pred.distance = std::sqrt(dx*dx + dz*dz);
                 pred.last_throw_pos = player_pos;
            } else if (!throws_.empty()) {
                 pred.last_throw_pos = throws_.back().overworld_position();
            }
            
            for (const auto& t : throws_) {
                core::Vec2d pos = t.overworld_position();
                pred.angle_errors.push_back(
                    ranked_[i].angle_error(pos, t.corrected_angle(), version_)
                );
            }
            
            if (throws_.size() >= 2 && !pred.angle_errors.empty()) {
                double outlier_score = compute_outlier_score(pred.angle_errors);
                
                if (outlier_score > mismeasure_threshold) {
                    pred.likely_mismeasure = true;
                    pred.warning_message = "Possible mismeasure detected (MAD score: " + 
                        std::to_string(static_cast<int>(outlier_score * 10) / 10.0) + ")";
                    pred.certainty *= std::max(0.6, 1.0 - (outlier_score - mismeasure_threshold) * 0.05);
                }
            }
            
            if (is_poor_triangulation) {
                pred.poor_triangulation = true;
                if (pred.warning_message.empty()) {
                    pred.warning_message = "Parallel throws - move and throw again";
                } else {
                    pred.warning_message += "; parallel throws";
                }
                pred.certainty *= 0.7;
            }
                        
            results.push_back(std::move(pred));
        }
        
        return results;
    }

    
    [[nodiscard]] double highest_certainty() const {
        return ranked_.empty() ? 0.0 : ranked_[0].probability;
    }
    
    [[nodiscard]] const WorldChunk* best_chunk() const {
        return ranked_.empty() ? nullptr : &ranked_[0];
    }

private:
    void compute_posterior() {
        if (throws_.empty()) return;

        const auto& first_throw = throws_[0];
        double sigma0 = get_throw_std_dev(first_throw);
        
        double tolerance_deg = std::min(1.0, 30.0 * sigma0);
        double tolerance_rad = tolerance_deg * core::coords::DEG_TO_RAD;
        
        ProbabilityPrior prior(first_throw, tolerance_rad, version_, divine_context_);
        chunks_ = prior.candidates();
        if (chunks_.empty()) return;
        
        for (const auto& t : throws_) {
            condition(t);
        }
        
        if (use_advanced_) {
            apply_closest_stronghold_condition();
        }
        
        std::sort(chunks_.begin(), chunks_.end(),
            [](const WorldChunk& a, const WorldChunk& b) {
                return a.probability > b.probability;
            });
        
        ranked_ = chunks_;
    }
    
    void condition(const EyeThrow& t) {
        for (auto& chunk : chunks_) {
            update_conditional_probability(chunk, t);
        }
        
        double weight_sum = 0.0;
        for (const auto& chunk : chunks_) {
            weight_sum += chunk.probability;
        }
        
        if (weight_sum > 0.0) {
            for (auto& chunk : chunks_) {
                chunk.probability /= weight_sum;
            }
        }
    }
    
    void apply_closest_stronghold_condition() {
        if (throws_.empty()) return;
        
        const auto& first_throw = throws_[0];
        double player_x = first_throw.x_overworld();
        double player_z = first_throw.z_overworld();
        
        std::sort(chunks_.begin(), chunks_.end(),
            [](const WorldChunk& a, const WorldChunk& b) {
                return a.probability > b.probability;
            });
        
        double total_prob = 0.0;
        int samples = 0;
        
        for (size_t i = 0; i < chunks_.size(); ++i) {
            auto& chunk = chunks_[i];
            
            if (i < 100 || chunk.probability > ClosestStrongholdConditioner::PROBABILITY_THRESHOLD) {
                ClosestStrongholdConditioner::ChunkData data{
                    chunk.pos.x,
                    chunk.pos.z,
                    chunk.probability
                };
                
                double factor = ClosestStrongholdConditioner::condition_chunk(
                    data, player_x, player_z, version_
                );
                
                chunk.probability *= factor;
                total_prob += factor;
                samples++;
            } else {
                chunk.probability *= (samples > 0) ? (total_prob / samples) : 0.0;
            }
        }
        
        double weight_sum = 0.0;
        for (const auto& chunk : chunks_) {
            weight_sum += chunk.probability;
        }
        
        if (weight_sum > 0.0) {
            for (auto& chunk : chunks_) {
                chunk.probability /= weight_sum;
            }
        }
    }
    
    void update_conditional_probability(WorldChunk& chunk, const EyeThrow& t) {
        int stronghold_offset = get_stronghold_chunk_coord(version_);
        double delta_x = chunk.pos.x * 16.0 + stronghold_offset - t.overworld_position().x;
        double delta_z = chunk.pos.z * 16.0 + stronghold_offset - t.overworld_position().z;
        
        double gamma = -180.0 / core::coords::PI * std::atan2(delta_x, delta_z);
        
        double delta = std::fmod(std::abs(gamma - t.corrected_angle()), 360.0);
        delta = std::min(delta, 360.0 - delta);
        
        double sigma = get_throw_std_dev(t);
        double variance = sigma * sigma;
        
        double distance_squared = delta_x * delta_x + delta_z * delta_z;
        variance += get_variance_from_position_imprecision(distance_squared, t);
        
        chunk.probability *= std::exp(-delta * delta / (2.0 * variance));
    }
    
    double get_variance_from_position_imprecision(double distance_squared, const EyeThrow& t) {
        if (distance_squared < 100.0) return 0.0;
        
        double distance = std::sqrt(distance_squared);
        
        constexpr double MAX_POSITION_ERROR = 0.5;
        
        double angular_error_rad = std::atan(MAX_POSITION_ERROR / distance);
        double angular_error_deg = angular_error_rad * 180.0 / core::coords::PI;
        
        return angular_error_deg * angular_error_deg / 3.0;
    }

    
    core::Vec2d compute_center_estimate() {
        if (throws_.size() == 1) {
            const auto& t = throws_[0];
            core::Vec2d pos = t.overworld_position();
            core::Vec2d dir = t.direction();
            return pos + dir * 1500.0;
        }
        
        if (throws_.size() >= 2) {
            return triangulate_intersection();
        }
        
        return {0.0, 0.0};
    }
    
    core::Vec2d triangulate_intersection() {
        if (throws_.size() < 2) return {0.0, 0.0};
        
        size_t best_i = 0, best_j = 1;
        double best_cross = 0.0;
        
        for (size_t i = 0; i < throws_.size(); ++i) {
            for (size_t j = i + 1; j < throws_.size(); ++j) {
                core::Vec2d d1 = throws_[i].direction();
                core::Vec2d d2 = throws_[j].direction();
                double cross = std::abs(d1.x * d2.z - d1.z * d2.x);
                if (cross > best_cross) {
                    best_cross = cross;
                    best_i = i;
                    best_j = j;
                }
            }
        }
        
        const auto& t1 = throws_[best_i];
        const auto& t2 = throws_[best_j];
        
        core::Vec2d p1 = t1.overworld_position();
        core::Vec2d p2 = t2.overworld_position();
        core::Vec2d d1 = t1.direction();
        core::Vec2d d2 = t2.direction();
        
        double cross = d1.x * d2.z - d1.z * d2.x;
        if (std::abs(cross) < 1e-10) {
            return (p1 + p2) * 0.5 + d1 * 1500.0;
        }
        
        double t = ((p2.x - p1.x) * d2.z - (p2.z - p1.z) * d2.x) / cross;
        return p1 + d1 * std::max(0.0, t);
    }
    
    int compute_search_radius() {
        if (throws_.size() == 1) {
            return 100;
        }
        return 50;
    }
    
    double get_throw_std_dev(const EyeThrow& t) const {
        if (t.is_boat_mode) {
            return std_dev_boat_;
        }
        if (t.type == ThrowType::Manual) {
            return std_dev_manual_;
        }
        return std_dev_;
    }
    
    double compute_confidence_scale() const {
        if (throws_.empty()) return 0.0;
        if (ranked_.empty()) return 0.0;
        
        size_t n = throws_.size();
        double raw_prob = ranked_[0].probability;
        
        if (n == 1) {
            return raw_prob;
        }
        
        double best_cross = compute_best_cross_product();
        
        double triangulation_quality;
        if (best_cross < 0.05) {
            triangulation_quality = 0.50;
        } else if (best_cross < 0.15) {
            triangulation_quality = 0.50 + (best_cross - 0.05) * 3.0;
        } else if (best_cross < 0.30) {
            triangulation_quality = 0.80 + (best_cross - 0.15) * 1.0;
        } else if (best_cross < 0.50) {
            triangulation_quality = 0.95 + (best_cross - 0.30) * 0.2;
        } else {
            triangulation_quality = 0.99;
        }
        
        double throw_bonus = 0.0;
        if (n >= 3) {
            throw_bonus = std::min(0.05, (n - 2) * 0.02);
        }
        
        double result = raw_prob * triangulation_quality + throw_bonus;
        
        return std::min(1.0, result);
    }
    
    double compute_best_cross_product() const {
        double best_cross = 0.0;
        for (size_t i = 0; i < throws_.size(); ++i) {
            for (size_t j = i + 1; j < throws_.size(); ++j) {
                core::Vec2d d1 = throws_[i].direction();
                core::Vec2d d2 = throws_[j].direction();
                double cross = std::abs(d1.x * d2.z - d1.z * d2.x);
                if (cross > best_cross) best_cross = cross;
            }
        }
        return best_cross;
    }
    
    double get_max_std_dev() const {
        double max_std = std_dev_;
        for (const auto& t : throws_) {
            double sd = get_throw_std_dev(t);
            if (sd > max_std) max_std = sd;
        }
        return max_std;
    }
    
    double compute_geometry_quality() const {
        if (throws_.size() < 2) return 1.0;
        
        double best_cross = compute_best_cross_product();
        
        double min_dist = 1e9;
        for (size_t i = 0; i < throws_.size(); ++i) {
            for (size_t j = i + 1; j < throws_.size(); ++j) {
                double dx = throws_[i].position.x - throws_[j].position.x;
                double dz = throws_[i].position.z - throws_[j].position.z;
                double dist = std::sqrt(dx*dx + dz*dz);
                min_dist = std::min(min_dist, dist);
            }
        }
        
        double angle_quality = std::min(1.0, best_cross / 0.3);
        double distance_quality = std::min(1.0, min_dist / 50.0);
        
        double combined = 0.7 * angle_quality + 0.3 * distance_quality;
        
        return 0.85 + 0.15 * combined;
    }
    
    double compute_probability_concentration() const {
        if (ranked_.size() < 2) return 1.0;
        
        double top1 = ranked_[0].probability;
        double top5_sum = 0.0;
        int count = std::min(5, static_cast<int>(ranked_.size()));
        for (int i = 0; i < count; ++i) {
            top5_sum += ranked_[i].probability;
        }
        
        if (top5_sum < 1e-10) return 0.0;
        return top1 / top5_sum;
    }
    
    double compute_base_confidence(double concentration) const {
        size_t n_throws = throws_.size();
        double raw_prob = ranked_.empty() ? 0.0 : ranked_[0].probability;
        
        double throw_factor;
        if (n_throws == 1) {
            throw_factor = 0.12;
        } else if (n_throws == 2) {
            throw_factor = 0.96;
        } else {
            throw_factor = std::min(1.0, 0.96 + (n_throws - 2) * 0.02);
        }
        
        double concentration_boost = 0.7 + 0.3 * concentration;
        
        double geometry_quality = compute_geometry_quality();
        
        double confidence = raw_prob * throw_factor * concentration_boost * geometry_quality;
        
        return std::min(1.0, confidence);
    }
    
    static double compute_outlier_score(const std::vector<double>& errors) {
        if (errors.size() < 2) return 0.0;
        
        std::vector<double> abs_errors;
        abs_errors.reserve(errors.size());
        for (double e : errors) {
            abs_errors.push_back(std::abs(e));
        }
        
        std::sort(abs_errors.begin(), abs_errors.end());
        
        double median;
        size_t n = abs_errors.size();
        if (n % 2 == 0) {
            median = (abs_errors[n/2 - 1] + abs_errors[n/2]) / 2.0;
        } else {
            median = abs_errors[n/2];
        }
        
        std::vector<double> deviations;
        deviations.reserve(n);
        for (double e : abs_errors) {
            deviations.push_back(std::abs(e - median));
        }
        std::sort(deviations.begin(), deviations.end());
        
        double mad;
        if (n % 2 == 0) {
            mad = (deviations[n/2 - 1] + deviations[n/2]) / 2.0;
        } else {
            mad = deviations[n/2];
        }
        
        mad = std::max(0.5, mad);
        
        double max_error = *std::max_element(abs_errors.begin(), abs_errors.end());
        
        if (max_error < 2.0) return 0.0;
        
        double modified_z_score = 0.6745 * (max_error - median) / mad;
        
        return std::max(0.0, modified_z_score);
    }

    
    ChunkList group_neighboring_chunks(const ChunkList& sorted_chunks) {
        if (sorted_chunks.empty()) return {};
        
        ChunkList grouped;
        std::map<std::pair<int, int>, size_t> pos_to_idx;
        std::vector<bool> merged(sorted_chunks.size(), false);
        
        for (size_t i = 0; i < sorted_chunks.size(); ++i) {
            pos_to_idx[{sorted_chunks[i].pos.x, sorted_chunks[i].pos.z}] = i;
        }
        
        for (size_t i = 0; i < sorted_chunks.size(); ++i) {
            if (merged[i]) continue;
            
            WorldChunk combined = sorted_chunks[i];
            merged[i] = true;
            
            std::vector<size_t> to_merge;
            to_merge.push_back(i);
            
            size_t processed = 0;
            while (processed < to_merge.size()) {
                size_t current = to_merge[processed++];
                const auto& current_chunk = sorted_chunks[current];
                
                static const int dx[] = {1, -1, 0, 0, 1, 1, -1, -1};
                static const int dz[] = {0, 0, 1, -1, 1, -1, 1, -1};
                
                for (int k = 0; k < 8; ++k) {
                    auto it = pos_to_idx.find({current_chunk.pos.x + dx[k], current_chunk.pos.z + dz[k]});
                    if (it != pos_to_idx.end()) {
                        size_t neighbor_idx = it->second;
                        if (!merged[neighbor_idx]) {
                            combined.probability += sorted_chunks[neighbor_idx].probability;
                            merged[neighbor_idx] = true;
                            to_merge.push_back(neighbor_idx);
                        }
                    }
                }
            }
            
            grouped.push_back(combined);
            if (grouped.size() >= 100) break;
        }
        
        std::sort(grouped.begin(), grouped.end(),
            [](const WorldChunk& a, const WorldChunk& b) {
                return a.probability > b.probability;
            });
        
        return grouped;
    }
    
    std::vector<EyeThrow> throws_;
    double std_dev_;
    double std_dev_boat_;
    double std_dev_manual_;
    bool use_advanced_;
    core::McVersion version_;
    DivineContext divine_context_;
    ChunkList chunks_;
    ChunkList ranked_;
};

}
