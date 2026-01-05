#pragma once

#include <string>
#include <optional>
#include <regex>
#include "dolbot/domain/eye_throw.hpp"

namespace dolbot::io {

struct ParsedF3C {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double yaw = 0.0;
    double pitch = 0.0;
    domain::Dimension dimension = domain::Dimension::Overworld;
};

class F3CParser {
public:
    static std::optional<ParsedF3C> parse(const std::string& input) {
        static const std::regex pattern(
            R"(^/execute in (\w+:\w+) run tp @s (-?[\d.]+) (-?[\d.]+) (-?[\d.]+) (-?[\d.]+) (-?[\d.]+)\s*$)"
        );
        
        std::smatch match;
        if (!std::regex_match(input, match, pattern)) {
            if (input.empty() || (!std::isdigit(static_cast<unsigned char>(input[0])) && input[0] != '-' && !std::isspace(static_cast<unsigned char>(input[0])))) {
                return std::nullopt;
            }
            return try_parse_1_12(input);
        }
        
        if (match.size() < 7) return std::nullopt;
        
        ParsedF3C result;
        
        std::string dim_str = match[1].str();
        if (dim_str == "minecraft:the_nether") {
            result.dimension = domain::Dimension::Nether;
        } else if (dim_str == "minecraft:the_end") {
            result.dimension = domain::Dimension::End;
        } else {
            result.dimension = domain::Dimension::Overworld;
        }
        
        try {
            result.x = std::stod(match[2].str());
            result.y = std::stod(match[3].str());
            result.z = std::stod(match[4].str());
            result.yaw = std::stod(match[5].str());
            result.pitch = std::stod(match[6].str());
        } catch (...) {
            return std::nullopt;
        }
        
        return result;
    }
    
    static std::optional<domain::EyeThrow> to_throw(const ParsedF3C& data, double crosshair_correction = 0.0) {
        domain::EyeThrow throw_data;
        throw_data.position = {data.x, data.z};
        throw_data.horizontal_angle = data.yaw;
        throw_data.vertical_angle = data.pitch;
        throw_data.dimension = data.dimension;
        throw_data.type = domain::ThrowType::Normal;
        throw_data.correction = crosshair_correction;
        
        return throw_data;
    }
    
    static std::optional<domain::EyeThrow> parse_to_throw(const std::string& input, 
                                                          double crosshair_correction = 0.0) {
        auto parsed = parse(input);
        if (!parsed) return std::nullopt;
        return to_throw(*parsed, crosshair_correction);
    }
    
    static bool is_looking_down(const ParsedF3C& data) {
        return data.pitch > 31.0;
    }

private:
    static std::optional<ParsedF3C> try_parse_1_12(const std::string& input) {
        static const std::regex pattern_1_12(
            R"(^\s*(-?\d{1,}\.\d+|-?\d{4,})\s+(-?\d{1,}\.\d+|-?\d{2,})\s+(-?\d{1,}\.\d+|-?\d{4,})\s+(-?\d{1,}\.\d+)\s+(-?\d{1,}\.\d+)\s*$)"
        );
        
        std::smatch match;
        if (!std::regex_match(input, match, pattern_1_12)) {
            return std::nullopt;
        }
        
        ParsedF3C result;
        result.dimension = domain::Dimension::Overworld;
        
        try {
            result.x = std::stod(match[1].str());
            result.y = std::stod(match[2].str());
            result.z = std::stod(match[3].str());
            result.yaw = std::stod(match[4].str());
            result.pitch = std::stod(match[5].str());
        } catch (...) {
            return std::nullopt;
        }
        
        return result;
    }
};

} // namespace dolbot::io
