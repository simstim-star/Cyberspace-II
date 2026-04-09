#pragma once

#include "render_types.h"

#define IS_LIGHT_ACTIVE(mask, i) (((mask) >> (i)) & 1)

namespace Sendai
{
class Renderer;
class Camera;
class Scene;

VOID LightsInit(Sendai::Scene &Scene, const Sendai::Camera &Camera);
VOID UpdateLights(BYTE ActiveLightMask, const Sendai::Light *const InLights, Sendai::Light *const OutLights,
                  UINT NumLights);

VOID RenderLightBillboards(Sendai::Renderer &Renderer, const Sendai::Light *Lights, BYTE ActiveLightMask,
                           Sendai::MeshConstants &MeshConstants);
} // namespace Sendai
