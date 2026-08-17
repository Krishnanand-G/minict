#pragma once
#include <string>
#include <vector>

namespace minict {

std::string trim(const std::string& s);
std::vector<std::string> split(const std::string& s, char delimiter);
bool ensure_dir(const std::string& path);
bool write_file(const std::string& path, const std::string& contents);
std::string read_file(const std::string& path);
long long now_ms();
std::string json_escape(const std::string& value);
bool run_process(const std::vector<std::string>& args);

}
