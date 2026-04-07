#pragma once
#include "render_types.h"

#define IS_LIGHT_ACTIVE(mask, i) (((mask) >> (i)) & 1)

namespace Sendai {
class Renderer;
class Camera;
class Scene;
} // namespace Sendai

VOID R_LightsInit(Sendai::Scene &Scene, const Sendai::Camera &Camera);
VOID R_UpdateLights(BYTE ActiveLightMask, const Sendai::Light *const InLights, Sendai::Light *const OutLights, UINT NumLights);

VOID R_RenderLightBillboards(Sendai::Renderer *const Renderer,
							 const Sendai::Light *Lights,
							 BYTE ActiveLightMask,
							 Sendai::MeshConstants *const MeshConstants);
