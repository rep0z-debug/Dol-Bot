#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QIcon>
#include <QTimer>

#include "dolbot/core/logger.hpp"
#include "dolbot/core/config.hpp"
#include "dolbot/domain/approx_density.hpp"
#include "dolbot/ui/main_panel.hpp"
#include "dolbot/ui/theme_engine.hpp"
#include "dolbot/ui/theme_library.hpp"

constexpr const char* APP_VERSION = "1.0.1";

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    app.setApplicationName("Dol Bot");
    app.setApplicationVersion(APP_VERSION);
    app.setOrganizationName("DolBot");
    app.setWindowIcon(QIcon(":/icons/app"));
    
    LOG_INFO("Dol Bot starting...");
    LOG_INFO(std::string("Version: ") + APP_VERSION);
    
    dolbot::core::Config::instance().load();
    dolbot::core::Config::instance().set(&dolbot::core::AppSettings::fake_coords_enabled, false);
    LOG_INFO("Configuration loaded");
    
    dolbot::domain::ApproxDensity::instance();
    LOG_INFO("Density tables initialized");
    
    auto& engine = dolbot::ui::ThemeEngine::instance();
    std::string saved_theme = dolbot::core::Config::instance().settings().theme;
    auto theme = dolbot::ui::ThemeLibrary::instance().load_theme(QString::fromStdString(saved_theme));
    if (theme.has_value()) {
        engine.apply_theme(theme.value());
        LOG_INFO("Theme applied: " + saved_theme);
    } else {
        engine.apply_dark_theme();
        LOG_INFO("Theme applied: Dark (default)");
    }
    
    dolbot::ui::MainPanel main_window;
    main_window.show();
    
    LOG_INFO("Application ready");
    
    return app.exec();
}
