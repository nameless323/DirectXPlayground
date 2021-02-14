#pragma once

#include "Scene/Scene.h"

namespace DirectxPlayground
{
class DummyScene : public Scene
{
public:
    void InitResources(RenderContext& context) override;
    void Render(RenderContext& context) override;
};
}