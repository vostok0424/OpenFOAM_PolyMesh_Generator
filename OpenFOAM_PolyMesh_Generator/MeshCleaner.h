
#pragma once

#include "MeshTypes.h"

// 清理未被任何面使用的点，并压缩点编号。
// - 只保留被 faces 引用的点，按原顺序压缩
// - 更新 faces 中的点索引
// - owner / neighbour / patch 信息都不变
void removeUnusedPoints(MeshData& mesh);
