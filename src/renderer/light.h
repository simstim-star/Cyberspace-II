#pragma once
#include "render_types.h"

#define IS_LIGHT_ACTIVE(mask, i) (((mask) >> (i)) & 1)

struct S_Scene;
struct R_Core;

namespace Sendai {
class Camera;
}

void R_LightsInit(S_Scene *const Scene, const Sendai::Camera *const Camera);
void R_UpdateLights(BYTE ActiveLightMask, const R_Light *const InLights, R_Light *const OutLights, UINT NumLights);
void R_RenderLightBillboards(R_Core *const Renderer, const R_Light *Lights, BYTE ActiveLightMask, R_MeshConstants *const MeshConstants);
