#pragma once
#include <DirectXMathC.h>

namespace Sendai
{
class Renderer;
class MeshConstants;

VOID CreateGrid(Sendai::Renderer &const Renderer, const float HalfSide);
VOID RenderGrid(Sendai::Renderer &const Renderer, Sendai::MeshConstants &MeshConstants);
} // namespace Sendai