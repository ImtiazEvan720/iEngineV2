#ifndef IENGINEV2_SCRIPTCOMPONENT_H
#define IENGINEV2_SCRIPTCOMPONENT_H

#include "components/Component.h"

#include <string>

class ScriptComponent : public Component {
public:
    explicit ScriptComponent(std::string scriptPath);

    const std::string& getScriptPath() const;
    bool isLoaded() const;

    void onStart() override;
    void onUpdate(float deltaTime) override;
    void onDestroy() override;

private:
    std::string scriptPath;
    bool loaded = false;
};

#endif
