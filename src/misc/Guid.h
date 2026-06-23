#pragma once

#include <string>

class Guid final {
public:
    static std::string generate();
    static bool isValid(const std::string& value);

private:
    Guid() = delete;
};
