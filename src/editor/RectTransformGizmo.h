#pragma once
#include "editor/EditorCamera.h"
#include "editor/RectGizmo.h"

class EntityInspectorPanel;
class EditorCamera;
class RectTransformComponent;

class RectTransformGizmo {

public:
  void draw(const EntityInspectorPanel &entityInspector,
            const EditorCamera &camera) const;

private:
  RectGizmo::Rect rect = RectGizmo::Rect();
  RectGizmo rectGizmo;
  void calculateRect(RectGizmo::Rect &rect,const RectTransformComponent &rectTransform) const;
  void applyRect(const RectGizmo::Rect &rect,
                 RectTransformComponent &rectTransform);
};