#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include "dolbot/core/signal.hpp"
#include "dolbot/domain/eye_throw.hpp"
#include "dolbot/domain/probability_posterior.hpp"
#include "dolbot/domain/world_chunk.hpp"
#include "dolbot/domain/divine_context.hpp"

namespace dolbot::domain {

struct TriangulationResult {
    std::vector<PredictionResult> predictions;
    double computation_time_ms = 0.0;
    int throw_count = 0;
    bool locked = false;
    bool has_divine = false;
    
    [[nodiscard]] bool has_result() const {
        return !predictions.empty();
    }
    
    [[nodiscard]] const PredictionResult* best() const {
        return predictions.empty() ? nullptr : &predictions[0];
    }
    
    [[nodiscard]] double top_certainty() const {
        return predictions.empty() ? 0.0 : predictions[0].certainty;
    }
    
    [[nodiscard]] bool has_any_warning() const {
        for (const auto& pred : predictions) {
            if (pred.has_warning()) return true;
        }
        return false;
    }
};

class Triangulator {
public:
    Triangulator() = default;
    
    void set_settings(double std_dev, double boat_std_dev, double manual_std_dev, bool use_advanced, core::McVersion version, double mismeasure_threshold = 3.0) {
        std_dev_ = std_dev;
        std_dev_boat_ = boat_std_dev;
        std_dev_manual_ = manual_std_dev;
        use_advanced_ = use_advanced;
        version_ = version;
        mismeasure_threshold_ = mismeasure_threshold;
    }
    
    void set_divine_context(const DivineContext& ctx) {
        divine_context_ = ctx;
        recalculate();
    }
    
    void set_fossil(const Fossil& fossil) {
        divine_context_.set_fossil(fossil);
        recalculate();
    }
    
    void clear_divine() {
        divine_context_.clear();
        recalculate();
    }
    
    [[nodiscard]] bool has_divine() const {
        return divine_context_.has_divine();
    }
    
    bool add_throw(const EyeThrow& throw_data) {
        if (locked_) return false;
        
        if (!throws_.empty()) {
            const auto& last = throws_.back();
            if (std::abs(last.position.x - throw_data.position.x) < 0.01 &&
                std::abs(last.position.z - throw_data.position.z) < 0.01 &&
                std::abs(last.horizontal_angle - throw_data.horizontal_angle) < 0.01) {
                return false;
            }
        }
        
        redo_stack_.clear();
        throws_.push_back(throw_data);
        recalculate();
        return true;
    }
    
    bool add_throw(EyeThrow&& throw_data) {
        if (locked_) return false;

        if (!throws_.empty()) {
            const auto& last = throws_.back();
            if (std::abs(last.position.x - throw_data.position.x) < 0.01 &&
                std::abs(last.position.z - throw_data.position.z) < 0.01 &&
                std::abs(last.horizontal_angle - throw_data.horizontal_angle) < 0.01) {
                return false;
            }
        }

        redo_stack_.clear();
        throws_.push_back(std::move(throw_data));
        recalculate();
        return true;
    }
    
    bool undo_last() {
        if (locked_ || throws_.empty()) return false;
        redo_stack_.push_back(std::move(throws_.back()));
        throws_.pop_back();
        recalculate();
        return true;
    }
    
    bool remove_throw(int index) {
        if (locked_ || index < 0 || index >= static_cast<int>(throws_.size())) return false;
        
        redo_stack_.push_back(std::move(throws_[index]));
        throws_.erase(throws_.begin() + index);
        recalculate();
        return true;
    }
    
    bool redo_last() {
        if (locked_ || redo_stack_.empty()) return false;
        throws_.push_back(std::move(redo_stack_.back()));
        redo_stack_.pop_back();
        recalculate();
        return true;
    }
    
    void set_std_dev(double val, double boat_val, double manual_val) {
        if (std_dev_ == val && std_dev_boat_ == boat_val && std_dev_manual_ == manual_val) return;
        std_dev_ = val;
        std_dev_boat_ = boat_val;
        std_dev_manual_ = manual_val;
        recalculate();
    }
    
    void reset() {
        if (locked_) return;
        for (auto it = throws_.rbegin(); it != throws_.rend(); ++it) {
            redo_stack_.push_back(std::move(*it));
        }
        throws_.clear();
        result_ = {};
        result_changed_.fire(result_);
    }
    
    void full_reset() {
        locked_ = false;
        throws_.clear();
        redo_stack_.clear();
        divine_context_.clear();
        result_ = {};
        result_changed_.fire(result_);
    }
    
    void lock() {
        locked_ = true;
        result_.locked = true;
        result_changed_.fire(result_);
    }
    
    void unlock() {
        locked_ = false;
        result_.locked = false;
        result_changed_.fire(result_);
    }
    
    [[nodiscard]] bool is_locked() const { return locked_; }
    
    void adjust_last_angle(double delta) {
        if (throws_.empty()) return;
        throws_.back().apply_correction(delta);
        recalculate();
    }
    
    void toggle_alt_std_on_last() {
        if (throws_.empty()) return;
        auto& last = throws_.back();
        if (last.type == ThrowType::Normal) {
            last.type = ThrowType::Manual;
        } else if (last.type == ThrowType::Manual) {
            last.type = ThrowType::Normal;
        }
        recalculate();
    }
    
    [[nodiscard]] const TriangulationResult& result() const { return result_; }
    [[nodiscard]] const std::vector<EyeThrow>& throws() const { return throws_; }
    [[nodiscard]] const std::vector<EyeThrow>& redo_stack() const { return redo_stack_; }
    [[nodiscard]] int throw_count() const { return static_cast<int>(throws_.size()); }
    
    void set_redo_stack(std::vector<EyeThrow> stack) {
        redo_stack_ = std::move(stack);
    }
    [[nodiscard]] const DivineContext& divine_context() const { return divine_context_; }
    
    core::ConnectionHandle on_result_changed(std::function<void(const TriangulationResult&)> callback) {
        return result_changed_.connect(std::move(callback));
    }
    
    void update_player_position(const core::Vec2d& pos) {
        current_position_ = pos;
    }
    
    [[nodiscard]] std::optional<double> distance_to_stronghold() const {
        if (!result_.has_result() || !current_position_) return std::nullopt;
        const auto* best = result_.best();
        if (!best) return std::nullopt;
        return best->chunk.distance_blocks(*current_position_, version_);
    }
    
    [[nodiscard]] std::optional<double> angle_to_stronghold() const {
        if (!result_.has_result() || !current_position_) return std::nullopt;
        const auto* best = result_.best();
        if (!best) return std::nullopt;
        return best->chunk.angle_to(*current_position_, version_);
    }
    
    [[nodiscard]] std::optional<core::Vec2d> get_stronghold_position() const {
        if (!result_.has_result()) return std::nullopt;
        const auto* best = result_.best();
        if (!best) return std::nullopt;
        return core::Vec2d{
            static_cast<double>(best->chunk.stronghold_x(version_)),
            static_cast<double>(best->chunk.stronghold_z(version_))
        };
    }

private:
    void recalculate() {
        if (throws_.empty()) {
            result_ = {};
            result_.locked = locked_;
            result_.has_divine = divine_context_.has_divine();
            result_changed_.fire(result_);
            return;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        ProbabilityPosterior posterior(
            throws_, 
            std_dev_, 
            std_dev_boat_, 
            std_dev_manual_, 
            use_advanced_, 
            version_,
            divine_context_
        );
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::optional<core::Vec2d> last_pos = std::nullopt;
        if (!throws_.empty()) {
            last_pos = throws_.back().overworld_position();
        }
        
        result_.predictions = posterior.top_predictions(5, mismeasure_threshold_, last_pos);
        result_.computation_time_ms = duration.count() / 1000.0;
        result_.throw_count = static_cast<int>(throws_.size());
        result_.locked = locked_;
        result_.has_divine = divine_context_.has_divine();
        
        result_changed_.fire(result_);
    }
    
    std::vector<EyeThrow> throws_;
    std::vector<EyeThrow> redo_stack_;
    TriangulationResult result_;
    bool locked_ = false;
    double std_dev_ = 0.1;
    double std_dev_boat_ = 0.001;
    double std_dev_manual_ = 0.03;
    bool use_advanced_ = true;
    core::McVersion version_ = core::McVersion::V1_19_plus;
    double mismeasure_threshold_ = 3.0;
    std::optional<core::Vec2d> current_position_;
    DivineContext divine_context_;
    core::Signal<const TriangulationResult&> result_changed_;
};

}
