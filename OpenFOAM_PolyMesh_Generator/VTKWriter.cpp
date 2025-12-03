
#include "VTKWriter.h"

#include <fstream>
#include <iomanip>
#include <iostream>

void writeVTKSurface(const MeshData& mesh, const std::string& filePath)
{
    // 将整个面集合按 VTK POLYDATA 写出，便于在 ParaView 中快速检查拓扑
    const std::size_t nPoints = mesh.points.size();
    const std::size_t nFaces  = mesh.faces.size();

    std::ofstream out(filePath);
    if (!out)
    {
        std::cerr << "Cannot open VTK file for writing: " << filePath << std::endl;
        return;
    }

    out << "# vtk DataFile Version 3.0\n";
    out << "OpenFOAM mesh surface (faces only)\n";
    out << "ASCII\n";
    out << "DATASET POLYDATA\n";

    // 写 points
    out << "POINTS " << nPoints << " double\n";
    out << std::setprecision(16);
    for (const auto& p : mesh.points)
    {
        out << p.x << " " << p.y << " " << p.z << "\n";
    }

    // 写 faces 作为 POLYGONS
    // 每个面是四边形：行格式为: 4 idx0 idx1 idx2 idx3
    const std::size_t vertsPerFace = 4;
    const std::size_t listSize = nFaces * (1 + vertsPerFace); // 每面行首一个数字 + 4 个顶点索引

    out << "POLYGONS " << nFaces << " " << listSize << "\n";
    for (const auto& f : mesh.faces)
    {
        out << vertsPerFace;
        for (int k = 0; k < 4; ++k)
        {
            out << " " << f[k];
        }
        out << "\n";
    }

    out.close();
}
