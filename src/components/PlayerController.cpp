#include "components/PlayerController.h"
#include "components/TransformComponent.h"
#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "Entity.h"
#include "misc/Asset.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/Renderer.h"
#include "system/VirtualInputSystem.h"

#include <iostream>

void PlayerController::onStart() {
    std::cout << "PlayerController started." << std::endl;
    std::cout << "Entity name: " << getEntity()->getName() << ", tag: " << getEntity()->getTag() << std::endl;
    transform = getEntity()->getComponent<TransformComponent>();

    if (transform == nullptr) {
        std::cerr << "PlayerController requires a TransformComponent to function properly." << std::endl;
    }
    animator = getEntity()->getComponent<AnimationComponent>();
    if (animator == nullptr) {
        std::cerr << "PlayerController requires an AnimationComponent to function properly." << std::endl;
    }
}

void PlayerController::fire() {
    if (transform == nullptr) {
        std::cerr << "Cannot fire bullet because PlayerController has no TransformComponent." << std::endl;
        return;
    }

    TextureAsset* spriteSheetAsset = AssetManager::getInstance().getTextureAssetByName(
        "NES - Battle City (JPN) - Miscellaneous - General Sprites.png"
    );
    if (spriteSheetAsset == nullptr || spriteSheetAsset->getTextureHandle() == nullptr) {
        std::cerr << "Cannot fire bullet because the sprite sheet texture was not found." << std::endl;
        return;
    }

    const float height = 16.0f;
    const float width = 16.0f;
    const float renderScale = Renderer::getInstance().getRenderScale();
    const float bulletWidth = (width / 2.0f) * renderScale;
    const float bulletHeight = height * renderScale;

    Level& level = Level::getCurrentLevel();
    Entity& bulletEntity = level.createEntity();

    Sprite bullet(
        spriteSheetAsset->getTextureHandle(),
        RenderRect{width * 20.0f, height * 6.0f, width / 2.0f, height}
    );
    bullet.setSize(Vector2F(bulletWidth, bulletHeight));

    bulletEntity.setName("Bullet" + std::to_string(firedBullets++));
    bulletEntity.setTag("Bullet");
    bulletEntity.addComponent<TransformComponent>(transform->getPosition(), transform->getRotation());
    bulletEntity.addComponent<SpriteComponent>(bullet);
    bulletEntity.addComponent<CollisionComponent>(
        bulletWidth,
        bulletHeight,
        CollisionComponent::BodyType::Dynamic,
        true,
        "Bullet"
    );
    bulletEntity.addComponent<ScriptComponent>("Assets/Scripts/bullet.lua");
}

void PlayerController::onUpdate(float deltaTime) { 
    // std::cout << "PlayerController updating. Delta time: " << deltaTime << " seconds." << std::endl; 

    if (transform == nullptr) {
        return;
    }

    VirtualInputSystem& input = VirtualInputSystem::getInstance();

    if (input.wasActionPressed(InputAction::Fire)) {
        fire();
    }

    Vector2F position = transform->getPosition();
    float rotation = transform->getRotation();
    bool moving = false;
    const float speed = 100.0f;

    if (input.isActionDown(InputAction::MoveUp)) {
        position.y -= speed * deltaTime;
        rotation = 0.0f;
        moving = true;
    } else if (input.isActionDown(InputAction::MoveDown)) {
        position.y += speed * deltaTime;
        rotation = 180.0f;
        moving = true;
    } else if (input.isActionDown(InputAction::MoveLeft)) {
        position.x -= speed * deltaTime;
        rotation = -90.0f;
        moving = true;
    } else if (input.isActionDown(InputAction::MoveRight)) {
        position.x += speed * deltaTime;
        rotation = 90.0f;
        moving = true;
    }

    if (moving) {

        if(animator != nullptr) {
            animator->play();
        } else {
            std::cerr << "PlayerController is moving but has no AnimationComponent to play." << std::endl;
        }

        transform->setPosition(position);
        transform->setRotation(rotation);
    }
    else {
        if(animator != nullptr) {
            if(animator->isPlaying()) {
                std::cout << "PlayerController stopped moving. Pausing animation." << std::endl;
            }   
            animator->pause();
        }
    }   
}

std::unique_ptr<Component> PlayerController::clone() const {
    return std::make_unique<PlayerController>();
}
