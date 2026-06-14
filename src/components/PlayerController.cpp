#include "components/PlayerController.h"
#include "components/TransformComponent.h"
#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/SpriteComponent.h"
#include "Entity.h"
#include "game/Bullet.h"
#include "misc/Asset.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/Renderer.h"

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

void PlayerController::onKeyPressed(sf::Keyboard::Key key) {
    std::cout << "PlayerController key pressed: " << static_cast<int>(key) << std::endl;
    if(key == sf::Keyboard::Key::W) {
        isMovingUp = 1;
        isMovingLeft = 0;
        isMoving = true;
    } else if(key == sf::Keyboard::Key::S) {
        isMovingUp = -1;
        isMovingLeft = 0;
        isMoving = true;
    } else if(key == sf::Keyboard::Key::A) {
        isMovingLeft = 1;
        isMovingUp = 0;
        isMoving = true;
    } else if(key == sf::Keyboard::Key::D) {
        isMovingLeft = -1;
        isMovingUp = 0;
        isMoving = true;
    } else if(key == sf::Keyboard::Key::Space) {
        fire();
    }
}

void PlayerController::onKeyReleased(sf::Keyboard::Key key) {
    std::cout << "PlayerController key released: " << static_cast<int>(key) << std::endl;

    if(key == sf::Keyboard::Key::W || key == sf::Keyboard::Key::S || key == sf::Keyboard::Key::A || key == sf::Keyboard::Key::D) {
        isMoving = false;
    }
          
}

void PlayerController::onMousePressed(sf::Mouse::Button button, int x, int y) {
    std::cout << "PlayerController mouse pressed: " << static_cast<int>(button)
              << " at " << x << ", " << y << std::endl;
}

void PlayerController::onMouseReleased(sf::Mouse::Button button, int x, int y) {
    std::cout << "PlayerController mouse released: " << static_cast<int>(button)
              << " at " << x << ", " << y << std::endl;
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
    bulletEntity.addComponent<Bullet>();
}

void PlayerController::onUpdate(float deltaTime) { 
    // std::cout << "PlayerController updating. Delta time: " << deltaTime << " seconds." << std::endl; 

    if (isMoving && transform != nullptr) {

        if(animator != nullptr) {
            animator->play();
        } else {
            std::cerr << "PlayerController is moving but has no AnimationComponent to play." << std::endl;
        }

        Vector2F position = transform->getPosition();
        float rotation = 0.0f;
        if (isMovingUp > 0) {
            position.y -= 100.0f * deltaTime;
            rotation = 0.0f;
        }
        
        if (isMovingUp < 0) {
            position.y += 100.0f * deltaTime;
            rotation = 180.0f;
        }

        if (isMovingLeft > 0) {
            position.x -= 100.0f * deltaTime;
            rotation = -90.0f;
        }
        
        if (isMovingLeft < 0) {
            position.x += 100.0f * deltaTime;
            rotation = 90.0f;
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
