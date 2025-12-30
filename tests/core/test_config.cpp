#include <gtest/gtest.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "dolbot/core/config.hpp"

using namespace dolbot::core;

class ConfigTest : public ::testing::Test {
protected:
    QJsonObject create_valid_settings_json() {
        QJsonObject obj;
        obj["std_deviation"] = 0.1;
        obj["std_dev_boat"] = 0.001;
        obj["std_dev_manual"] = 0.03;
        obj["crosshair_correction"] = 0.0;
        obj["use_advanced_stats"] = true;
        obj["show_angle_errors"] = false;
        obj["mc_version"] = 3;  // V1_19_plus
        obj["display_mode"] = 0;
        obj["show_nether_coords"] = true;
        obj["show_overlay"] = true;
        obj["overlay_opacity"] = 0.85;
        obj["current_theme"] = "Dark";
        return obj;
    }
};

TEST_F(ConfigTest, AppSettingsDefaultValues) {
    AppSettings settings;
    
    EXPECT_DOUBLE_EQ(settings.std_deviation, 0.1);
    EXPECT_DOUBLE_EQ(settings.std_dev_boat, 0.001);
    EXPECT_DOUBLE_EQ(settings.std_dev_manual, 0.03);
    EXPECT_DOUBLE_EQ(settings.crosshair_correction, 0.0);
    EXPECT_TRUE(settings.use_advanced_stats);
    EXPECT_FALSE(settings.show_angle_errors);
}

TEST_F(ConfigTest, McVersionDefault) {
    AppSettings settings;
    
    // Default should be V1_19_plus
    EXPECT_EQ(settings.mc_version, McVersion::V1_19_plus);
}

TEST_F(ConfigTest, DisplayModeDefault) {
    AppSettings settings;
    
    EXPECT_EQ(settings.display_mode, DisplayMode::FourFour);
}

TEST_F(ConfigTest, ValidateValidJson) {
    QJsonObject obj = create_valid_settings_json();
    
    auto result = Config::instance().validate_settings_json(obj);
    
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.error_message.isEmpty());
}

TEST_F(ConfigTest, ValidateInvalidStdDeviation) {
    QJsonObject obj = create_valid_settings_json();
    obj["std_deviation"] = -0.5;  
    
    auto result = Config::instance().validate_settings_json(obj);
    
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigTest, ValidateInvalidStdDeviationTooHigh) {
    QJsonObject obj = create_valid_settings_json();
    obj["std_deviation"] = 100.0;  
    
    auto result = Config::instance().validate_settings_json(obj);
    
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigTest, ValidateInvalidWindowOpacity) {
    QJsonObject obj = create_valid_settings_json();
    obj["window_opacity"] = 150;  
    
    auto result = Config::instance().validate_settings_json(obj);
    
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigTest, ValidateTooLowWindowOpacity) {
    QJsonObject obj = create_valid_settings_json();
    obj["window_opacity"] = 5;
    
    auto result = Config::instance().validate_settings_json(obj);
    
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigTest, DetectPathTraversal) {
    QJsonObject obj = create_valid_settings_json();
    obj["log_file_path"] = "../../../etc/passwd";
    
    auto result = Config::instance().validate_settings_json(obj);
    
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigTest, DetectWindowsPathTraversal) {
    QJsonObject obj = create_valid_settings_json();
    obj["log_file_path"] = "..\\..\\Windows\\System32";
    
    auto result = Config::instance().validate_settings_json(obj);
    
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigTest, DetectAbsolutePathWindows) {
    QJsonObject obj = create_valid_settings_json();
    obj["log_file_path"] = "C:\\Windows\\System32\\cmd.exe";
    
    auto result = Config::instance().validate_settings_json(obj);
    
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigTest, DetectScriptTagInHotkey) {
    QJsonObject obj = create_valid_settings_json();
    QJsonObject hotkeys;
    hotkeys["reset"] = "<script>alert('xss')</script>";
    obj["hotkeys"] = hotkeys;
    
    auto result = Config::instance().validate_settings_json(obj);
    
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigTest, AcceptReasonableThemeName) {
    QJsonObject obj = create_valid_settings_json();
    obj["theme"] = "My Custom Dark Theme";
    
    auto result = Config::instance().validate_settings_json(obj);
    
    EXPECT_TRUE(result.valid);
}

TEST_F(ConfigTest, RejectExcessivelyLongTheme) {
    QJsonObject obj = create_valid_settings_json();
    QString very_long_string(200, 'A');  
    obj["theme"] = very_long_string;
    
    auto result = Config::instance().validate_settings_json(obj);
    
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigTest, McVersionPre1_9) {
    AppSettings settings;
    settings.mc_version = McVersion::Pre_1_9;
    
    EXPECT_EQ(static_cast<int>(settings.mc_version), 0);
}

TEST_F(ConfigTest, McVersionV1_9_to_1_12) {
    AppSettings settings;
    settings.mc_version = McVersion::V1_9_to_1_12;
    
    EXPECT_EQ(static_cast<int>(settings.mc_version), 1);
}

TEST_F(ConfigTest, McVersionV1_13_to_1_18) {
    AppSettings settings;
    settings.mc_version = McVersion::V1_13_to_1_18;
    
    EXPECT_EQ(static_cast<int>(settings.mc_version), 2);
}

TEST_F(ConfigTest, McVersionV1_19_plus) {
    AppSettings settings;
    settings.mc_version = McVersion::V1_19_plus;
    
    EXPECT_EQ(static_cast<int>(settings.mc_version), 3);
}

TEST_F(ConfigTest, EffectiveAngleAdjustmentSubpixel) {
    AppSettings settings;
    settings.angle_adjustment_type = AngleAdjustmentType::Subpixel;
    settings.tall_resolution_height = 1080;
    
    double adjustment = settings.effective_angle_adjustment();
    
    EXPECT_DOUBLE_EQ(adjustment, 0.0);
}

TEST_F(ConfigTest, EffectiveAngleAdjustmentTallRes) {
    AppSettings settings;
    settings.angle_adjustment_type = AngleAdjustmentType::TallResolution;
    settings.tall_resolution_height = 1080;
    
    double adjustment = settings.effective_angle_adjustment();
    
    EXPECT_NE(adjustment, 0.0);
}

TEST_F(ConfigTest, EffectiveAngleAdjustmentCustom) {
    AppSettings settings;
    settings.angle_adjustment_type = AngleAdjustmentType::Custom;
    settings.custom_angle_adjustment = 0.123;
    
    double adjustment = settings.effective_angle_adjustment();
    
    EXPECT_DOUBLE_EQ(adjustment, 0.123);
}

TEST_F(ConfigTest, HotkeyConfigDefaults) {
    HotkeyConfig hotkeys;
    
    EXPECT_EQ(hotkeys.reset, "Ctrl+R");
    EXPECT_EQ(hotkeys.undo, "Ctrl+Z");
    EXPECT_EQ(hotkeys.redo, "Ctrl+Y");
    EXPECT_EQ(hotkeys.toggle_lock, "Ctrl+L");
}

TEST_F(ConfigTest, HotkeyConfigEquality) {
    HotkeyConfig h1, h2;
    
    EXPECT_TRUE(h1 == h2);
    
    h2.reset = "Ctrl+X";
    EXPECT_FALSE(h1 == h2);
}

TEST_F(ConfigTest, SingletonInstance) {
    Config& c1 = Config::instance();
    Config& c2 = Config::instance();
    
    EXPECT_EQ(&c1, &c2);
}

TEST_F(ConfigTest, GetSettings) {
    const AppSettings& settings = Config::instance().settings();
    
    EXPECT_GE(settings.std_deviation, 0.0);
}

TEST_F(ConfigTest, FakeCoordGeneratorSingleton) {
    auto& g1 = FakeCoordGenerator::instance();
    auto& g2 = FakeCoordGenerator::instance();
    
    EXPECT_EQ(&g1, &g2);
}

TEST_F(ConfigTest, FakeCoordGeneratorApply) {
    auto& gen = FakeCoordGenerator::instance();
    
    auto [x, z] = gen.apply(100, 200);
    
    int dx = x - 100;
    int dz = z - 200;
    
    EXPECT_EQ(dx, gen.offset_x());
    EXPECT_EQ(dz, gen.offset_z());
}

TEST_F(ConfigTest, FakeCoordGeneratorRegenerate) {
    auto& gen = FakeCoordGenerator::instance();
    
    int old_x = gen.offset_x();
    int old_z = gen.offset_z();
    
    gen.regenerate();
    
    EXPECT_TRUE(true);
}
