#pragma once

#include "misc/Asset.h"

#include <string>

class PrefabAsset : public Asset {
public:
    PrefabAsset(std::string name, std::string path);

    bool load() override;
};
