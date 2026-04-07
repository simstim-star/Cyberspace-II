#pragma once

#include "../renderer/render_types.h"
#include "memory.h"
#include <vector>

struct R_Model;
struct ID3D12RootSignature;

namespace Sendai {
struct Scene {
	std::vector<Model> Models;

	SceneData Data;
	BYTE ActiveLightMask;

	M_Arena SceneArena;
	M_Arena UploadArena;
};
} // namespace Sendai