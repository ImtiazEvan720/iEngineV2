#ifndef IENGINEV2_SOUNDASSET_H
#define IENGINEV2_SOUNDASSET_H

#include "misc/Asset.h"

#include <SFML/Audio/SoundBuffer.hpp>

#include <memory>
#include <string>

class SoundAsset : public Asset {
public:
    SoundAsset(std::string name, std::string path);

    bool load() override;

    sf::SoundBuffer* getSoundBuffer();
    const sf::SoundBuffer* getSoundBuffer() const;

private:
    std::unique_ptr<sf::SoundBuffer> soundBuffer;
};

#endif
