#pragma once

#include <cmath>
#include <numbers>

namespace dolbot::core {

struct Vec2d {
    double x = 0.0;
    double z = 0.0;
    
    Vec2d() = default;
    Vec2d(double x_, double z_) : x(x_), z(z_) {}
    
    [[nodiscard]] double length() const {
        return std::sqrt(x * x + z * z);
    }
    
    [[nodiscard]] double length_squared() const {
        return x * x + z * z;
    }
    
    [[nodiscard]] double distance_to(const Vec2d& other) const {
        double dx = x - other.x;
        double dz = z - other.z;
        return std::sqrt(dx * dx + dz * dz);
    }
    
    [[nodiscard]] Vec2d normalized() const {
        double len = length();
        if (len == 0.0) return {0.0, 0.0};
        return {x / len, z / len};
    }
    
    Vec2d operator+(const Vec2d& other) const { return {x + other.x, z + other.z}; }
    Vec2d operator-(const Vec2d& other) const { return {x - other.x, z - other.z}; }
    Vec2d operator*(double scalar) const { return {x * scalar, z * scalar}; }
    Vec2d operator/(double scalar) const { return {x / scalar, z / scalar}; }
    
    bool operator==(const Vec2d& other) const {
        return x == other.x && z == other.z;
    }
};

struct ChunkPos {
    int x = 0;
    int z = 0;
    
    ChunkPos() = default;
    ChunkPos(int x_, int z_) : x(x_), z(z_) {}
    
    [[nodiscard]] int center_x() const { return x * 16 + 8; }
    [[nodiscard]] int center_z() const { return z * 16 + 8; }
    
    [[nodiscard]] int stronghold_x() const { return x * 16 + 4; }
    [[nodiscard]] int stronghold_z() const { return z * 16 + 4; }
    
    [[nodiscard]] int nether_x() const { return x * 2; }
    [[nodiscard]] int nether_z() const { return z * 2; }
    
    [[nodiscard]] int distance_squared(const ChunkPos& other) const {
        int dx = x - other.x;
        int dz = z - other.z;
        return dx * dx + dz * dz;
    }
    
    [[nodiscard]] bool is_neighbor(const ChunkPos& other) const {
        return std::abs(x - other.x) <= 1 && std::abs(z - other.z) <= 1;
    }
    
    bool operator==(const ChunkPos& other) const {
        return x == other.x && z == other.z;
    }
    
    bool operator!=(const ChunkPos& other) const {
        return !(*this == other);
    }
};

namespace coords {

constexpr double PI = std::numbers::pi;
constexpr double DEG_TO_RAD = PI / 180.0;
constexpr double RAD_TO_DEG = 180.0 / PI;

inline double to_radians(double degrees) {
    return degrees * DEG_TO_RAD;
}

inline double to_degrees(double radians) {
    return radians * RAD_TO_DEG;
}

inline double angle_from_coords(double dx, double dz) {
    return -std::atan2(dx, dz) * RAD_TO_DEG;
}

inline double phi_from_coords(double x, double z) {
    return -std::atan2(x, z);
}

inline double normalize_angle(double angle) {
    angle = std::fmod(angle, 360.0);
    if (angle < -180.0) angle += 360.0;
    if (angle > 180.0) angle -= 360.0;
    return angle;
}

inline double angle_difference(double a, double b) {
    return normalize_angle(a - b);
}

inline Vec2d overworld_to_nether(const Vec2d& pos) {
    return {pos.x / 8.0, pos.z / 8.0};
}

inline Vec2d nether_to_overworld(const Vec2d& pos) {
    return {pos.x * 8.0, pos.z * 8.0};
}

} // namespace coords

} // namespace dolbot::core
