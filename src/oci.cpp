#include "oci.hpp"
#include "rootfs.hpp"
#include "config.hpp"
#include "util.hpp"
#include <cstdlib>
#include <sstream>
#include <vector>

namespace minict {

static std::string rootfs_dir(const std::string& name) {
    return rootfs_path(name);
}

static std::string digest_to_blob(const std::string& digest) {
    size_t colon = digest.find(':');
    std::string hex = colon == std::string::npos ? digest : digest.substr(colon + 1);
    return "blobs/sha256/" + hex;
}

static std::vector<std::string> find_digests(const std::string& json) {
    std::vector<std::string> out;
    std::string needle = "\"digest\"";
    size_t pos = 0;
    while ((pos = json.find(needle, pos)) != std::string::npos) {
        pos = json.find(':', pos);
        if (pos == std::string::npos) break;
        ++pos;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
        if (pos >= json.size() || json[pos] != '"') { ++pos; continue; }
        ++pos;
        size_t end = json.find('"', pos);
        if (end == std::string::npos) break;
        out.push_back(json.substr(pos, end - pos));
        pos = end + 1;
    }
    return out;
}

static std::string resolve_layout_dir(const std::string& path, bool sim) {
    if (path.size() > 4 && path.substr(path.size() - 4) == ".tar") {
        std::string tmp = state_dir() + "/oci-extract";
        ensure_dir(state_dir());
        ensure_dir(tmp);
        if (sim) return path;
#ifdef __linux__
        std::string cmd = "tar -xf '" + path + "' -C '" + tmp + "'";
        if (std::system(cmd.c_str()) != 0) return "";
        return tmp;
#else
        return "";
#endif
    }
    return path;
}

OciResult load_oci(const std::string& in_path, const std::string& name, bool sim) {
    std::string dest = rootfs_dir(name);
    ensure_dir(state_dir());
    ensure_dir(state_dir() + "/rootfs");
    ensure_dir(dest);

    std::string layout = resolve_layout_dir(in_path, sim);
    if (layout.empty()) {
        return OciResult(false, dest, "could not extract oci archive");
    }

    std::string index_path = layout + "/index.json";
    std::string index = read_file(index_path);
    if (index.empty()) {
        return OciResult(false, dest, "missing index.json");
    }

    std::vector<std::string> manifests = find_digests(index);
    if (manifests.empty()) {
        return OciResult(false, dest, "no manifest in index");
    }

    std::string manifest_path = layout + "/" + digest_to_blob(manifests[0]);
    std::string manifest = read_file(manifest_path);
    if (manifest.empty()) {
        return OciResult(false, dest, "manifest blob missing");
    }

    std::vector<std::string> layers;
    std::vector<std::string> all_digests = find_digests(manifest);
    // first digest in manifest is usually config; rest are layers
    for (size_t i = 1; i < all_digests.size(); ++i) layers.push_back(all_digests[i]);
    if (layers.empty() && !all_digests.empty()) layers.push_back(all_digests.back());

    if (sim) {
        std::ostringstream marker;
        marker << "oci from: " << in_path << "\n";
        for (size_t i = 0; i < layers.size(); ++i) marker << layers[i] << "\n";
        bool ok = write_file(dest + "/.minict-rootfs", marker.str());
        return OciResult(ok, dest, ok ? "oci marker created" : "unable to write marker");
    }

#ifdef __linux__
    for (size_t i = 0; i < layers.size(); ++i) {
        std::string blob = layout + "/" + digest_to_blob(layers[i]);
        std::string cmd = "tar -xf '" + blob + "' -C '" + dest + "' 2>/dev/null || "
                          "tar -xzf '" + blob + "' -C '" + dest + "'";
        if (std::system(cmd.c_str()) != 0) {
            return OciResult(false, dest, "layer unpack failed: " + layers[i]);
        }
    }
    return OciResult(true, dest, "oci image loaded");
#else
    return OciResult(false, dest, "oci load requires Linux or MINICT_SIM=1");
#endif
}

}
