#pragma once
#include "embedded/application/Preferences.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <utility>
namespace cluster::host {
inline std::filesystem::path preferencePath() {
    if(const auto* overridePath=std::getenv("R15_SETTINGS_PATH")) return overridePath;
    const auto* userHome=std::getenv("HOME");
    if(!userHome) return {}; // No unexpected fallback into the working directory.
#ifdef __APPLE__
    return std::filesystem::path(userHome)/"Library/Application Support/R15 Smart Cluster/preferences.bin";
#else
    if(const auto* config=std::getenv("XDG_CONFIG_HOME")) return std::filesystem::path(config)/"r15-smart-cluster/preferences.bin";
    return std::filesystem::path(userHome)/".config/r15-smart-cluster/preferences.bin";
#endif
}
// Host-only storage adapter. No filesystem/streams in the portable model or UI.
class PreferenceFile {
public:
    explicit PreferenceFile(std::filesystem::path path):path_(std::move(path)) {}
    bool load(Preferences& value) const {
        value={};
        std::ifstream input(path_,std::ios::binary);
        PreferenceBytes bytes{};
        input.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
        const auto size=std::size_t(input.gcount());
        if(!input.eof()&&input.peek()!=std::char_traits<char>::eof())return false;
        return decodePreferences(bytes.data(),size,value);
    }
    bool save(const Preferences& value) const {
        Preferences validated;
        if(path_.empty()||!decodePreferences(encodePreferences(value),validated)) return false;
        std::error_code error;
        if(!path_.parent_path().empty()) std::filesystem::create_directories(path_.parent_path(),error);
        if(error) return false;
        auto temporary=path_; temporary+=".tmp";
        const auto bytes=encodePreferences(value);
        {
            std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
            if(!output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size())) return false;
            output.flush();
            if(!output) return false;
            output.close();
            if(!output) return false;
        }
        std::filesystem::rename(temporary,path_,error);
        if(error) { std::error_code ignored; std::filesystem::remove(temporary,ignored); return false; }
        return true;
    }
private:
    std::filesystem::path path_;
};
}
