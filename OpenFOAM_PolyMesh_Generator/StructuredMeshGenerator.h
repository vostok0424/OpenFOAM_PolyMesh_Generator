
#pragma once

#include "MeshTypes.h"

// 生成规则结构六面体网格（要求 X-Y 平面为正方形单元）
MeshData generateStructuredMesh(int Nx, int Ny, int Nz,
                                double Lx, double Ly, double Lz);
