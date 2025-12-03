
#pragma once

#include <string>
#include "MeshTypes.h"

// 以 VTK legacy POLYDATA 格式写出所有 faces（用于在 ParaView 中快速检查拓扑）
void writeVTKSurface(const MeshData& mesh, const std::string& filePath);

