#include "misc/MusicAsset.h"

#include <iostream>
#include <memory>
#include <utility>

MusicAsset::MusicAsset(std::string name, std::string path)
    : Asset(std::move(name), std::move(path), Type::Music) {}

bool MusicAsset::load() {
    music = std::make_unique<sf::Music>();
    setLoaded(music->openFromFile(getPath()));

    if (!isLoaded()) {
        std::cerr << "Failed to load " << typeToString(getType()) << ": " << getPath() << std::endl;
    }

    return isLoaded();
}

sf::Music* MusicAsset::getMusic() {
    return music.get();
}

const sf::Music* MusicAsset::getMusic() const {
    return music.get();
}
