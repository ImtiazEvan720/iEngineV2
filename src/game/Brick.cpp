#include "game/Brick.h"
#include "math/Vector2F.h"
#include "misc/Animation.h"
#include "misc/Sprite.h"
#include "misc/Level.h"
#include "components/AnimationComponent.h"
#include "components/SpriteComponent.h"
#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/Renderer.h"

#include "Entity.h"

#include <iostream>
#include <memory>

void Brick::onStart() {
    collision = getEntity()->getComponent<CollisionComponent>();
    if (collision == nullptr) {
        std::cerr << "Brick requires a CollisionComponent." << std::endl;
        return;
    }
    collision->setListener(this); 
}

void Brick::onDestroy() {
    if (collision != nullptr) {
        collision->setListener(nullptr);
        collision = nullptr;
    }
}

void Brick::onCollisionEnter(CollisionComponent& self, CollisionComponent& other) {
    if (destroying) {
        return;
    }

    std::cout << "Brick collision: " << self.getName()
              << " touched " << other.getName() << std::endl;

    TextureAsset* spriteSheetAsset = AssetManager::getInstance().getTextureAssetByName(
        "NES - Battle City (JPN) - Miscellaneous - General Sprites.png"
    );
    if (spriteSheetAsset == nullptr || spriteSheetAsset->getTextureHandle() == nullptr) {
        std::cerr << "Failed to find Battle City sprite sheet texture." << std::endl;
        return;
    }

    const float width = 16.0f;
    const float height = 16.0f;
    const float renderScale = Renderer::getInstance().getRenderScale();

    Animation animation(0.1f);
    Vector2F startIndex(16.0f, 8.0f); // Starting index for brick sprites in the sprite sheet
    for(int i = startIndex.x; i < startIndex.x + 3; ++i) {
        Sprite brickSprite(
            spriteSheetAsset->getTextureHandle(),
            RenderRect{width * i, height * startIndex.y, width, height}
        );
        brickSprite.setSize(Vector2F(width * renderScale, height * renderScale));
        animation.addFrame(brickSprite);
    }

    destroyAnimation = &getEntity()->addComponent<AnimationComponent>(animation);
    destroyAnimation->setLooping(false);
    destroyAnimation->play();
    destroying = true;

    if (collision != nullptr) {
        collision->onDestroy();
        collision->setListener(nullptr);
        collision = nullptr;
    }
}

void Brick::onUpdate(float deltaTime) {
    (void)deltaTime;

    if (destroying && destroyAnimation != nullptr && destroyAnimation->isFinished()) {
        Level::getCurrentLevel().destroyEntity(getEntity());
    }
}
