cmake_minimum_required(VERSION 3.20)
project(DolBot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(BUILD_TESTS "Build unit tests" OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Svg Network)

add_library(dolbot_core STATIC
    include/dolbot/core/signal.hpp
    include/dolbot/core/coords.hpp
    include/dolbot/core/config.hpp
    include/dolbot/core/logger.hpp
    src/core/signal.cpp
    src/core/coords.cpp
    src/core/config.cpp
    src/core/logger.cpp
)
target_include_directories(dolbot_core PUBLIC include)
target_link_libraries(dolbot_core PUBLIC Qt6::Core)

add_library(dolbot_domain STATIC
    include/dolbot/domain/fortress_ring.hpp
    include/dolbot/domain/world_chunk.hpp
    include/dolbot/domain/eye_throw.hpp
    include/dolbot/domain/probability_prior.hpp
    include/dolbot/domain/probability_posterior.hpp
    include/dolbot/domain/triangulator.hpp
    include/dolbot/domain/blind_evaluator.hpp
    include/dolbot/domain/fossil_divine.hpp
    include/dolbot/domain/approx_density.hpp
    include/dolbot/domain/throw_advisor.hpp
    src/domain/fortress_ring.cpp
    src/domain/world_chunk.cpp
    src/domain/eye_throw.cpp
    src/domain/probability_prior.cpp
    src/domain/probability_posterior.cpp
    src/domain/triangulator.cpp
    src/domain/blind_evaluator.cpp
    src/domain/fossil_divine.cpp
    src/domain/approx_density.cpp
)
target_include_directories(dolbot_domain PUBLIC include)
target_link_libraries(dolbot_domain PUBLIC dolbot_core)

add_library(dolbot_io STATIC
    include/dolbot/io/clipboard_watcher.hpp
    include/dolbot/io/hotkey_service.hpp
    include/dolbot/io/settings_store.hpp
    include/dolbot/io/f3c_parser.hpp
    include/dolbot/io/log_watcher.hpp
    include/dolbot/io/update_checker.hpp
    include/dolbot/io/http_server.hpp
    include/dolbot/io/sound_manager.hpp
    src/io/clipboard_watcher.cpp
    src/io/hotkey_service.cpp
    src/io/settings_store.cpp
    src/io/f3c_parser.cpp
    src/io/log_watcher.cpp
    src/io/sound_manager.cpp
)
target_include_directories(dolbot_io PUBLIC include)
target_link_libraries(dolbot_io PUBLIC dolbot_core dolbot_domain Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)
if(WIN32)
    target_link_libraries(dolbot_io PUBLIC winmm)
endif()

add_library(dolbot_ui STATIC
    include/dolbot/ui/main_panel.hpp
    include/dolbot/ui/result_display.hpp
    include/dolbot/ui/settings_panel.hpp
    include/dolbot/ui/overlay_window.hpp
    include/dolbot/ui/throw_list.hpp
    include/dolbot/ui/blind_panel.hpp
    include/dolbot/ui/theme_engine.hpp
    include/dolbot/ui/theme_data.hpp
    include/dolbot/ui/theme_library.hpp
    include/dolbot/ui/theme_editor.hpp
    include/dolbot/ui/status_bar.hpp
    include/dolbot/ui/calibration_dialog.hpp
    include/dolbot/ui/divine_panel.hpp
    include/dolbot/ui/secure_label.hpp
    src/ui/main_panel.cpp
    src/ui/result_display.cpp
    src/ui/settings_panel.cpp
    src/ui/overlay_window.cpp
    src/ui/throw_list.cpp
    src/ui/blind_panel.cpp
    src/ui/theme_engine.cpp
    src/ui/theme_editor.cpp
    src/ui/status_bar.cpp
)
target_include_directories(dolbot_ui PUBLIC include)
target_link_libraries(dolbot_ui PUBLIC dolbot_core dolbot_domain dolbot_io Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Svg Qt6::Network)

set(APP_SOURCES src/main.cpp resources/resources.qrc)
if(WIN32)
    list(APPEND APP_SOURCES resources/app.rc)
endif()

add_executable(DolBot WIN32 ${APP_SOURCES})
target_link_libraries(DolBot PRIVATE 
    dolbot_core 
    dolbot_domain 
    dolbot_io 
    dolbot_ui
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Gui
    Qt6::Svg
    Qt6::Network
)

if(WIN32)
    target_link_libraries(dolbot_io PUBLIC user32)
    
    set_target_properties(DolBot PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

if(BUILD_TESTS)
    # Standalone tests
    add_executable(manual_test tests/standalone/manual_test.cpp)
    target_link_libraries(manual_test PRIVATE dolbot_domain dolbot_core)
    set_target_properties(manual_test PROPERTIES AUTOMOC OFF)
    
    add_executable(accuracy_test tests/standalone/accuracy_test.cpp)
    target_link_libraries(accuracy_test PRIVATE dolbot_domain dolbot_core)
    set_target_properties(accuracy_test PROPERTIES AUTOMOC OFF)
    
    # GTest based tests
    find_package(GTest QUIET)
    if(GTest_FOUND)
        add_executable(dolbot_tests
            # Core tests
            tests/core/test_coords.cpp
            tests/core/test_signal.cpp
            tests/core/test_config.cpp
            # Domain tests
            tests/domain/test_accuracy.cpp
            tests/domain/test_boat.cpp
            tests/domain/test_posterior.cpp
            tests/domain/test_ring_math.cpp
            tests/domain/test_triangulator.cpp
            tests/domain/test_versions.cpp
            tests/domain/test_fossil_divine.cpp
            tests/domain/test_throw_advisor.cpp
            tests/domain/test_blind_evaluator.cpp
            tests/domain/test_boat_travel.cpp
            tests/domain/test_eye_throw.cpp
            # IO tests
            tests/io/test_f3c.cpp
        )
        target_link_libraries(dolbot_tests PRIVATE 
            dolbot_domain 
            dolbot_core 
            dolbot_io
            GTest::gtest_main
            Qt6::Core
        )
        add_test(NAME DolBotTests COMMAND dolbot_tests)
    endif()
endif()

install(TARGETS DolBot DESTINATION bin)
install(DIRECTORY resources/icons resources/lang DESTINATION share/dolbot)


