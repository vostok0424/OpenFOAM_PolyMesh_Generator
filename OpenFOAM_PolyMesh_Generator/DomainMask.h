
#pragma once

#include <functional>
#include "MeshTypes.h"

// 掩模函数：给单元中心点，返回是否在计算域中
using MaskFunc = std::function<bool(const Point& cellCenter)>;

// 对背景规则网格应用掩模，返回裁剪后的非结构网格
// - bgMesh: 由 StructuredMeshGenerator 生成的完整盒子网格
// - inDomain: 掩模函数，true 表示该单元被保留
MeshData applyMask(const MeshData& bgMesh, MaskFunc inDomain);
