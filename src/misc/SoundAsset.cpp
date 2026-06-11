#include "misc/SoundAsset.h"

#include <iostream>
#include <memory>
#include <utility>

SoundAsset::SoundAsset(std::string name, std::string path)
    : Asset(std::move(name), std::move(path), Type::Sound) {}

bool SoundAsset::load() {
    soundBuffer = std::make_unique<sf::SoundBuffer>();
    setLoaded(soundBuffer->loadFromFile(getPath()));

    if (!isLoaded()) {
        std::cerr << "Failed to load " << typeToString(getType()) << ": " << getPath() << std::endl;
    }

    return isLoaded();
}

sf::SoundBuffer* SoundAsset::getSoundBuffer() {
    return soundBuffer.get();
}

const sf::SoundBuffer* SoundAsset::getSoundBuffer() const {
    return soundBuffer.get();
}
