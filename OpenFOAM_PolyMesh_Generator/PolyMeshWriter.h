
#pragma once

#include <string>
#include "MeshTypes.h"

// 把 MeshData 写成 OpenFOAM polyMesh 文件
void writePolyMesh(const MeshData &mesh, const std::string &baseDir);
