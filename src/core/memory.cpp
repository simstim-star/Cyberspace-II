#include "pch.h"


#include "memory.h"
#include "../error/error.h"

D3D12_GPU_VIRTUAL_ADDRESS
M_GpuAddress(ID3D12Resource *Resource, UINT64 Offset)
{
    return Resource->GetGPUVirtualAddress() + Offset;
}
