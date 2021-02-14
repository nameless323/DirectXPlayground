#pragma once

namespace DirectxPlayground
{
struct RenderContext;

class Scene
{
public:
    virtual void InitResources(RenderContext& context) abstract;
    virtual void Render(RenderContext& context) abstract;
    virtual ~Scene() = default;
};
}
