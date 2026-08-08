#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#ifdef UNIT_TEST
#include "MockLibnx.h"
#else
#include <switch.h>
#endif

class ConfigManager {
public:
    static ConfigManager& getInstance();
    static void init();

    bool load();
    bool save();

    const char* getApiToken();
    void setApiToken(const char* token);

    int getPort();
    void setPort(int port);

    long getLastUpdateCheck();
    void setLastUpdateCheck(long timestamp);

    bool getDebug();
    void setDebug(bool debug);

    // Gate core's capssc (GET /screenshot) and hiddbg (POST /button) service
    // init - both default on to match existing behavior, but a client that only
    // wants telemetry/automation (not remote control/screen capture) can turn
    // them off to shave a bit of core's steady-state resource usage. See
    // AGENTS.md Rule #6.
    bool getEnableScreenshot();
    void setEnableScreenshot(bool enable);

    bool getEnableInput();
    void setEnableInput(bool enable);

    void generateDefaultConfig();
    void generatePassphrase(char* out, size_t max_len);

    void setConfigPath(const char* path) { m_configPath = path; }

private:
    ConfigManager();
    char m_apiToken[128];
    int m_port;
    long m_lastUpdateCheck;
    bool m_debug;
    bool m_enableScreenshot;
    bool m_enableInput;
    const char* m_configPath = "sdmc:/config/NXRemoteAPI/settings.json";

    void generateRandomToken(char* out, size_t length);
};

#endif
