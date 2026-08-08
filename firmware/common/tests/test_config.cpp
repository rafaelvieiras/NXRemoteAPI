#include "catch.hpp"
#include "../include/ConfigManager.h"
#include <fstream>
#include <filesystem>

TEST_CASE("ConfigManager loads and saves correctly", "[config]") {
    // Setup temporary test path
    const std::string test_path = "test_settings.json";
    
    SECTION("Default config generation") {
        ConfigManager& cfg = ConfigManager::getInstance();
        cfg.setConfigPath("test_settings.json");
        cfg.generateDefaultConfig();
        
        REQUIRE(strlen(cfg.getApiToken()) > 0);
        // 8275 is ConfigManager's real default (see the constructor and
        // generateDefaultConfig() in ConfigManager.cpp) - this assertion used to
        // check 1337, which was never the actual default anywhere in the code.
        REQUIRE(cfg.getPort() == 8275);
    }

    SECTION("Loading missing config generates default") {
        // Ensure file doesn't exist
        std::filesystem::remove("test_settings.json");
        
        ConfigManager& cfg = ConfigManager::getInstance();
        cfg.setConfigPath("test_settings.json");
        bool res = cfg.load(); // This will try to create/save default
        
        REQUIRE(res == true);
        REQUIRE(strlen(cfg.getApiToken()) > 0);
        std::filesystem::remove("test_settings.json");
    }
}
