#pragma once

#include <cmath>
#include <vector>
#include <functional>
#include "dolbot/core/coords.hpp"
#include "dolbot/core/config.hpp"
#include "dolbot/domain/fortress_ring.hpp"

namespace dolbot::domain {

class WorldChunk {
public:
    core::ChunkPos pos;
    double probability = 0.0;
    
    WorldChunk() = default;
    WorldChunk(int x, int z) : pos(x, z) {}
    WorldChunk(int x, int z, double prob) : pos(x, z), probability(prob) {}
    WorldChunk(core::ChunkPos p) : pos(p) {}
    WorldChunk(core::ChunkPos p, double prob) : pos(p), probability(prob) {}
    
    [[nodiscard]] int stronghold_x(core::McVersion version = core::McVersion::V1_19_plus) const {
        return pos.x * 16 + get_stronghold_chunk_coord(version);
    }
    
    [[nodiscard]] int stronghold_z(core::McVersion version = core::McVersion::V1_19_plus) const {
        return pos.z * 16 + get_stronghold_chunk_coord(version);
    }
    
    [[nodiscard]] int distance_blocks(const core::Vec2d& from, core::McVersion version) const {
        double dx = stronghold_x(version) - from.x;
        double dz = stronghold_z(version) - from.z;
        return static_cast<int>(std::sqrt(dx * dx + dz * dz));
    }
    
    [[nodiscard]] double angle_to(const core::Vec2d& from, core::McVersion version) const {
        double dx = stronghold_x(version) - from.x;
        double dz = stronghold_z(version) - from.z;
        return core::coords::angle_from_coords(dx, dz);
    }
    
    [[nodiscard]] double angle_error(const core::Vec2d& throw_pos, double throw_angle, 
                                      core::McVersion version) const {
        double expected_angle = angle_to(throw_pos, version);
        return core::coords::normalize_angle(throw_angle - expected_angle);
    }
    
    [[nodiscard]] std::vector<double> compute_angle_errors(
        const std::vector<std::pair<core::Vec2d, double>>& throws,
        core::McVersion version) const {
        
        std::vector<double> errors;
        errors.reserve(throws.size());
        for (const auto& [pos, angle] : throws) {
            errors.push_back(angle_error(pos, angle, version));
        }
        return errors;
    }
    
    bool operator==(const WorldChunk& other) const {
        return pos == other.pos;
    }
    
    bool operator!=(const WorldChunk& other) const {
        return !(*this == other);
    }
};

struct ChunkHash {
    std::size_t operator()(const WorldChunk& c) const {
        return std::hash<int>()(c.pos.x) ^ (std::hash<int>()(c.pos.z) << 16);
    }
};

using ChunkList = std::vector<WorldChunk>;

} // namespace dolbot::domain
