#pragma once
#include <string>

namespace minict {

std::string socket_path();
bool socket_exists();

std::string json_get_string(const std::string& json, const std::string& key);
int json_get_int(const std::string& json, const std::string& key, int fallback);
bool json_get_bool(const std::string& json, const std::string& key, bool fallback);
std::string ipc_reply(bool ok, const std::string& detail, const std::string& body);

// connect, send one line, read one line; empty string on failure
std::string ipc_call(const std::string& request);

#ifdef __linux__
int ipc_listen_socket();
bool ipc_read_line(int fd, std::string& line);
bool ipc_write_line(int fd, const std::string& line);
#endif

}
