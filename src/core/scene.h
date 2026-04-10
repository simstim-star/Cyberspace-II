#pragma once

#include "../renderer/render_types.h"

namespace Sendai
{
struct Scene
{
    std::vector<Model> Models;

    SceneData Data;
    BYTE ActiveLightMask;
};
} // namespace Sendai