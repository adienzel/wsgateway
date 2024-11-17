#ifndef ENV_UTILS_H
#define ENV_UTILS_H

#include <cstdlib>
#include <string>
#include <stdexcept>

class EnvUtils {
public:
    static std::string getEnvString(const std::string& varName, const std::string& defaultValue) {
        const char* val = std::getenv(varName.c_str());
        if (val == nullptr || std::string(val).empty()) {
            return defaultValue;
        }
        return std::string(val);
    }

    static int getEnvInt(const std::string& varName, int defaultValue) {
        const char* val = std::getenv(varName.c_str());
        if (val == nullptr || std::string(val).empty()) {
            return defaultValue;
        }
        try {
            return std::stoi(val);
        } catch (const std::invalid_argument& e) {
            return defaultValue;
        } catch (const std::out_of_range& e) {
            return defaultValue;
        }
    }

    static float getEnvFloat(const std::string& varName, float defaultValue) {
        const char* val = std::getenv(varName.c_str());
        if (val == nullptr || std::string(val).empty()) {
            return defaultValue;
        }
        try {
            return std::stof(val);
        } catch (const std::invalid_argument& e) {
            return defaultValue;
        } catch (const std::out_of_range& e) {
            return defaultValue;
        }
    }
};

#endif // ENV_UTILS_H
