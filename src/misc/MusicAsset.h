#ifndef IENGINEV2_MUSICASSET_H
#define IENGINEV2_MUSICASSET_H

#include "misc/Asset.h"

#include <SFML/Audio/Music.hpp>

#include <memory>
#include <string>

class MusicAsset : public Asset {
public:
    MusicAsset(std::string name, std::string path);

    bool load() override;

    sf::Music* getMusic();
    const sf::Music* getMusic() const;

private:
    std::unique_ptr<sf::Music> music;
};

#endif
