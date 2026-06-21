#ifndef IENGINEV2_LEVELMANAGERPANEL_H
#define IENGINEV2_LEVELMANAGERPANEL_H

#include "misc/LevelManager.h"

#include <string>

class LevelManagerPanel {
public:
    bool draw(std::string& statusMessage);

private:
    bool drawLevelEntry(const LevelEntry& entry, std::string& statusMessage);
    bool loadLevel(const LevelEntry& entry, std::string& statusMessage);
    void processPendingRemove(std::string& statusMessage);

    std::string pendingRemoveFileName;
};

#endif
