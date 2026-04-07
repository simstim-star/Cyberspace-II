#pragma once
#include <DirectXMathC.h>

namespace Senai {
class Renderer;
class MeshConstants;
}

void R_CreateGrid(Sendai::Renderer *const Renderer, const float HalfSide);
void R_RenderGrid(Sendai::Renderer *const Renderer, Sendai::MeshConstants *const MeshConstants);