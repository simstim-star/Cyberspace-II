#pragma once

namespace Sendai
{
struct Renderer;

BOOL LoadModel(Sendai::Renderer *Renderer, const std::wstring &PathW, Sendai::Scene &Scene);
}
