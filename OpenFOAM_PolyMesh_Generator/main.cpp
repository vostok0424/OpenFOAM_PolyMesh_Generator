#include <string>
#include <cmath>
#include <iostream>
#include "StructuredMeshGenerator.h"
#include "PolyMeshWriter.h"
#include "DomainMask.h"
#include "VTKWriter.h"
#include "MeshCleaner.h"
#include <filesystem>

int main(int argc, char** argv)
{
    const int Nx = 1000;   // 比如激波管方向细一点
    const int Ny = 500;
    const int Nz = 1;

    const double Lx = 1.0;  // 0~Lx 方向布置：左边激波管，右边反射器
    const double Ly = 0.5;
    const double Lz = 0.01;

    std::string outDir = "polyMesh";
    if (argc > 1)
    {
        outDir = argv[1];
    }

    // 1) 生成背景结构网格
    MeshData bg = generateStructuredMesh(Nx, Ny, Nz, Lx, Ly, Lz);

    //------------------------------------------------------------------
    
    
    
    // 2) 定义“激波管 + 反射器”掩模
    //    这里只给个示例：左边 0<=x<=0.6, |y|<=0.1 为激波管；
    //    右边 0.6<x<=1.0 在某个半椭圆下方作为反射器区域，你可以按自己几何改。
    MaskFunc mask = [Lx, Ly](const Point& c) -> bool
    {
        // 直段激波管：x <= 0.6*Lx，保留全高 0..Ly
        const double tubeEnd = 0.5 * Lx;
        if (c.x <= tubeEnd)
        {
            return true;
        }

        // 反射器区域：使用一个以 (0.8*Lx, Ly/2) 为中心、
        // 半轴 a = 0.2*Lx, b = 0.5*Ly 的椭圆，刚好顶到 y=0 和 y=Ly
        double y0 = c.y - 0.5 * Ly;           // 平移到中轴
        double xc = (c.x - 0.5 * Lx) / (0.5 * Lx);  // a = 0.5 Lx
        double yc = y0 / (0.5 * Ly);                // b = 0.5 Ly

        if (c.x > tubeEnd && xc*xc + yc*yc <= Lx)
        {
            return true;
        }

        return false;
    };

    
    //------------------------------------------------------------------
    
    
    
    // 3) 应用掩模
    MeshData masked = applyMask(bg, mask);
    
    // 4) 清理未用节点
    removeUnusedPoints(masked);

    // 5) 输出网格
    writePolyMesh(masked, outDir);
    writeVTKSurface(masked, "mesh.vtk");

    return 0;
}
