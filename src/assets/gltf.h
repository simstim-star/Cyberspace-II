#pragma once
#include <string>

struct R_Model;

namespace Sendai {
class Renderer;
class Scene;
}

BOOL SendaiGLTF_LoadModel(Sendai::Renderer *Renderer, std::wstring &Path, Sendai::Scene &Scene);