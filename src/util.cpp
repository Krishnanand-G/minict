#include "util.hpp"
#include "config.hpp"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#endif

namespace minict {

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, d)) out.push_back(part);
    return out;
}

bool ensure_dir(const std::string& p) {
#ifdef _WIN32
    std::string cmd = "if not exist \"" + p + "\" mkdir \"" + p + "\"";
#else
    std::string cmd = "mkdir -p '" + p + "'";
#endif
    return std::system(cmd.c_str()) == 0;
}

bool write_file(const std::string& p, const std::string& s) {
    std::ofstream f(p.c_str(), std::ios::binary);
    if (!f) return false;
    f << s;
    return (bool)f;
}

std::string read_file(const std::string& p) {
    std::ifstream f(p.c_str(), std::ios::binary);
    std::stringstream s;
    s << f.rdbuf();
    return s.str();
}

long long now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

std::string json_escape(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '"' || c == '\\') out += '\\';
        if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

bool simulation_enabled() {
    const char* v = std::getenv("MINICT_SIM");
    return v && std::string(v) == "1";
}

std::string state_dir() {
    const char* v = std::getenv("MINICT_STATE_DIR");
    return v ? v : ".minict";
}

}
