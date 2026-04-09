#pragma once
#include <DirectXMath.h>

namespace Sendai
{
struct Renderer;
class MeshConstants;

VOID CreateGrid(Sendai::Renderer &Renderer, const FLOAT HalfSide);
VOID RenderGrid(Sendai::Renderer &Renderer, Sendai::MeshConstants &MeshConstants);
} // namespace Sendai