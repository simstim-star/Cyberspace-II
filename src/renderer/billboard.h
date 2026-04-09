#pragma once

#include "DirectXMath.h"

typedef struct BillboardVertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT2 UV;
} BillboardVertex;

extern BillboardVertex BillboardVertices[4];