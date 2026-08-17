#include "util.hpp"
#include "config.hpp"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <cerrno>

#ifdef __linux__
#include <sys/stat.h>
#else
#include <direct.h>
#endif
#include <sstream>

#ifdef __linux__
#include <sys/wait.h>
#include <unistd.h>
#endif

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
    if (p.empty()) return false;
    std::string current;
    size_t begin = 0;
    if (p[0] == '/') {
        current = "/";
        begin = 1;
    }
    for (size_t i = begin; i <= p.size(); ++i) {
        const bool boundary = i == p.size() || p[i] == '/' || p[i] == '\\';
        if (!boundary) {
            current += p[i];
            continue;
        }
        if (current.empty() || current == "/") {
            if (i < p.size()) current += p[i];
            continue;
        }
#ifdef _WIN32
        if (_mkdir(current.c_str()) != 0 && errno != EEXIST) return false;
#else
        if (mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) return false;
#endif
        if (i < p.size()) current += p[i];
    }
    return true;
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


bool run_process(const std::vector<std::string>& args) {
    if (args.empty()) return false;
#ifdef __linux__
    pid_t child = fork();
    if (child < 0) return false;
    if (child == 0) {
        std::vector<char*> argv;
        for (size_t i = 0; i < args.size(); ++i) {
            argv.push_back(const_cast<char*>(args[i].c_str()));
        }
        argv.push_back(0);
        execvp(argv[0], argv.data());
        _exit(127);
    }
    int status = 0;
    if (waitpid(child, &status, 0) < 0) return false;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#else
    return false;
#endif
}

}
