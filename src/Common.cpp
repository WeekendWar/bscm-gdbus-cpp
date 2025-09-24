#include "Common.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>

namespace Utils {

std::string bytes_to_hex_string(const std::vector<uint8_t>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
        if (&byte != &data.back()) {
            ss << " ";
        }
    }
    return ss.str();
}

std::vector<uint8_t> hex_string_to_bytes(const std::string& hex_str) {
    std::vector<uint8_t> bytes;
    std::string clean_hex = hex_str;
    
    // Remove spaces and make uppercase
    clean_hex.erase(std::remove(clean_hex.begin(), clean_hex.end(), ' '), clean_hex.end());
    std::transform(clean_hex.begin(), clean_hex.end(), clean_hex.begin(), ::toupper);
    
    // Convert hex pairs to bytes
    for (size_t i = 0; i < clean_hex.length(); i += 2) {
        if (i + 1 < clean_hex.length()) {
            std::string byte_str = clean_hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
            bytes.push_back(byte);
        }
    }
    
    return bytes;
}

std::string variant_to_string(GVariant* variant) {
    if (!variant) return "";
    
    const char* type_string = g_variant_get_type_string(variant);
    if (g_strcmp0(type_string, "s") == 0) {
        return std::string(g_variant_get_string(variant, nullptr));
    } else if (g_strcmp0(type_string, "o") == 0) {
        return std::string(g_variant_get_string(variant, nullptr));
    } else if (g_strcmp0(type_string, "b") == 0) {
        return g_variant_get_boolean(variant) ? "true" : "false";
    } else if (g_strcmp0(type_string, "n") == 0) {
        return std::to_string(g_variant_get_int16(variant));
    } else if (g_strcmp0(type_string, "i") == 0) {
        return std::to_string(g_variant_get_int32(variant));
    } else if (g_strcmp0(type_string, "u") == 0) {
        return std::to_string(g_variant_get_uint32(variant));
    } else if (g_strcmp0(type_string, "x") == 0) {
        return std::to_string(g_variant_get_int64(variant));
    } else if (g_strcmp0(type_string, "t") == 0) {
        return std::to_string(g_variant_get_uint64(variant));
    }
    
    return "<unsupported type: " + std::string(type_string) + ">";
}

std::vector<uint8_t> variant_to_bytes(GVariant* variant) {
    std::vector<uint8_t> bytes;
    
    if (!variant) return bytes;
    
    if (g_variant_is_of_type(variant, G_VARIANT_TYPE_BYTESTRING)) {
        gsize length;
        const guchar* data = reinterpret_cast<const guchar*>(g_variant_get_fixed_array(variant, &length, sizeof(guchar)));
        bytes.assign(data, data + length);
    } else if (g_variant_is_of_type(variant, G_VARIANT_TYPE("ay"))) {
        gsize length;
        const guchar* data = reinterpret_cast<const guchar*>(g_variant_get_fixed_array(variant, &length, sizeof(guchar)));
        bytes.assign(data, data + length);
    }
    
    return bytes;
}

void print_with_timestamp(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    struct tm* tm_info = localtime(&time_t);
    char timestamp[100];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    std::cout << "[" << timestamp << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
              << message << std::endl;
}

} // namespace Utils