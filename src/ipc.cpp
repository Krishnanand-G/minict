#include "ipc.hpp"
#include "config.hpp"
#include "util.hpp"
#include <cstdlib>
#include <cstring>
#include <sstream>

#ifdef __linux__
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace minict {

std::string socket_path() {
    return state_dir() + "/minict.sock";
}

bool socket_exists() {
#ifdef __linux__
    return access(socket_path().c_str(), F_OK) == 0;
#else
    return false;
#endif
}

static std::string find_json_value(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        ++pos;
        std::string out;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                ++pos;
                out += json[pos++];
            } else {
                out += json[pos++];
            }
        }
        return out;
    }
    size_t end = pos;
    while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ']') ++end;
    return trim(json.substr(pos, end - pos));
}

std::string json_get_string(const std::string& json, const std::string& key) {
    return find_json_value(json, key);
}

int json_get_int(const std::string& json, const std::string& key, int fallback) {
    std::string v = find_json_value(json, key);
    if (v.empty()) return fallback;
    return std::atoi(v.c_str());
}

bool json_get_bool(const std::string& json, const std::string& key, bool fallback) {
    std::string v = find_json_value(json, key);
    if (v == "true") return true;
    if (v == "false") return false;
    return fallback;
}

std::string ipc_reply(bool ok, const std::string& detail, const std::string& body) {
    std::ostringstream o;
    o << "{\"ok\":" << (ok ? "true" : "false")
      << ",\"detail\":\"" << json_escape(detail) << "\"";
    if (!body.empty()) o << ",\"body\":\"" << json_escape(body) << "\"";
    o << "}";
    return o.str();
}

#ifdef __linux__
static bool write_all(int fd, const std::string& s) {
    size_t sent = 0;
    while (sent < s.size()) {
        ssize_t n = write(fd, s.c_str() + sent, s.size() - sent);
        if (n <= 0) return false;
        sent += (size_t)n;
    }
    return true;
}

static std::string read_line_fd(int fd) {
    std::string out;
    char buf[256];
    while (true) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n <= 0) break;
        for (ssize_t i = 0; i < n; ++i) {
            if (buf[i] == '\n') return out;
            out += buf[i];
        }
    }
    return out;
}
#endif

std::string ipc_call(const std::string& request) {
#ifdef __linux__
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return "";

    sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::string path = socket_path();
    if (path.size() >= sizeof(addr.sun_path)) {
        close(fd);
        return "";
    }
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        return "";
    }

    std::string line = request;
    if (line.empty() || line[line.size() - 1] != '\n') line += "\n";
    if (!write_all(fd, line)) {
        close(fd);
        return "";
    }

    std::string resp = read_line_fd(fd);
    close(fd);
    return resp;
#else
    (void)request;
    return "";
#endif
}

#ifdef __linux__
int ipc_listen_socket() {
    ensure_dir(state_dir());
    std::string path = socket_path();
    std::remove(path.c_str());

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, 8) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

bool ipc_read_line(int fd, std::string& line) {
    line = read_line_fd(fd);
    return true;
}

bool ipc_write_line(int fd, const std::string& line) {
    std::string s = line;
    if (s.empty() || s[s.size() - 1] != '\n') s += "\n";
    return write_all(fd, s);
}

#endif

}
