#pragma once

#include "../renderer/render_types.h"
#include "memory.h"
#include <vector>

namespace Sendai
{
struct Scene
{
    std::vector<Model> Models;

    SceneData Data;
    BYTE ActiveLightMask;

    M_Arena SceneArena;
    M_Arena UploadArena;
};
} // namespace Sendai