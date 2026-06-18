#ifndef IENGINEV2_PREFABASSET_H
#define IENGINEV2_PREFABASSET_H

#include "misc/Asset.h"

#include <string>

class PrefabAsset : public Asset {
public:
    PrefabAsset(std::string name, std::string path);

    bool load() override;
};

#endif
