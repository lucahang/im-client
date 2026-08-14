#pragma once
#include <optional>
#include <string>
#include "message.pb.h"

class Codec {
public:
    static std::string Encode(const im::Message& msg);
    static std::optional<im::Message> Decode(const char* data, size_t len);
};