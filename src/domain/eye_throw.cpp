#include "dolbot/domain/eye_throw.hpp"
#include <regex>
#include <string>

namespace dolbot::domain {

std::optional<EyeThrow> parse_f3c_throw(const std::string& input, double crosshair_correction) {
    static const std::regex pattern(
        R"(/execute in (\w+:\w+) run tp @s (-?[\d.]+) (-?[\d.]+) (-?[\d.]+) (-?[\d.]+) (-?[\d.]+))"
    );
    
    std::smatch match;
    if (!std::regex_search(input, match, pattern) || match.size() < 7) {
        return std::nullopt;
    }
    
    EyeThrow result;
    
    std::string dim_str = match[1].str();
    if (dim_str == "minecraft:the_nether") {
        result.dimension = Dimension::Nether;
    } else if (dim_str == "minecraft:the_end") {
        result.dimension = Dimension::End;
    } else {
        result.dimension = Dimension::Overworld;
    }
    
    try {
        result.position.x = std::stod(match[2].str());
        result.position.z = std::stod(match[4].str());
        result.horizontal_angle = std::stod(match[5].str()) + crosshair_correction;
        result.vertical_angle = std::stod(match[6].str());
    } catch (...) {
        return std::nullopt;
    }
    
    return result;
}

} // namespace dolbot::domain
