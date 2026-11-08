#include "LinuxStorage.h"
#include <fstream>
#include <sstream>
#include <iostream>

LinuxStorage::LinuxStorage(const std::string& configFilePath)
    : _configFilePath(configFilePath) {}

bool LinuxStorage::begin() {
    loadFromFile();
    return true;
}

void LinuxStorage::loadFromFile() {
    std::lock_guard<std::mutex> lock(_mutex);
    std::ifstream infile(_configFilePath);
    if (!infile.is_open()) return;

    std::string line;
    while (std::getline(infile, line)) {
        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string k = line.substr(0, eqPos);
            std::string v = line.substr(eqPos + 1);
            _store[k] = v;
        }
    }
}

void LinuxStorage::persistToFile() {
    std::ofstream outfile(_configFilePath, std::ios::trunc);
    if (!outfile.is_open()) return;

    for (const auto& pair : _store) {
        outfile << pair.first << "=" << pair.second << "\n";
    }
}

bool LinuxStorage::setString(const char* key, const std::string& value) {
    if (!key) return false;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _store[key] = value;
    }
    persistToFile();
    return true;
}

std::string LinuxStorage::getString(const char* key, const std::string& defaultVal) {
    if (!key) return defaultVal;
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _store.find(key);
    if (it != _store.end()) return it->second;
    return defaultVal;
}

bool LinuxStorage::setUInt(const char* key, uint32_t value) {
    return setString(key, std::to_string(value));
}

uint32_t LinuxStorage::getUInt(const char* key, uint32_t defaultVal) {
    std::string valStr = getString(key, "");
    if (valStr.empty()) return defaultVal;
    try {
        return (uint32_t)std::stoul(valStr);
    } catch (...) {
        return defaultVal;
    }
}

bool LinuxStorage::setFloat(const char* key, float value) {
    return setString(key, std::to_string(value));
}

float LinuxStorage::getFloat(const char* key, float defaultVal) {
    std::string valStr = getString(key, "");
    if (valStr.empty()) return defaultVal;
    try {
        return std::stof(valStr);
    } catch (...) {
        return defaultVal;
    }
}

bool LinuxStorage::appendLog(const std::string& path, const std::string& line) {
    std::ofstream logFile(path, std::ios::app);
    if (!logFile.is_open()) return false;
    logFile << line << "\n";
    return true;
}

bool LinuxStorage::isSdMounted() const {
    return true;
}
