#pragma once
#include <string>

namespace Sendai
{
class Renderer;
class Scene;
} // namespace Sendai

BOOL SendaiGLTF_LoadModel(Sendai::Renderer *Renderer, std::wstring &Path, Sendai::Scene &Scene);