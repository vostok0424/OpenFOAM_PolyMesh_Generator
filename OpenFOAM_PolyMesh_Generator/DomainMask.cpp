
#include "DomainMask.h"
#include "StructuredMeshGenerator.h"

#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <cmath>

MeshData applyMask(const MeshData& bgMesh, MaskFunc inDomain)
{
    const int Nx = bgMesh.Nx;
    const int Ny = bgMesh.Ny;
    const int Nz = bgMesh.Nz;
    const int nCellsOld = Nx * Ny * Nz;

    auto cellIndex = [Nx, Ny](int i, int j, int k) -> int
    {
        return k * (Ny * Nx) + j * Nx + i;
    };

    auto pointIndex = [Nx, Ny](int i, int j, int k) -> int
    {
        return k * (Ny + 1) * (Nx + 1) + j * (Nx + 1) + i;
    };

    const auto& pts = bgMesh.points;

    // 1) 先根据单元中心决定哪些 cell 保留
    std::vector<char> keepCell(nCellsOld, 0);
    int newCellCount = 0;

    for (int k = 0; k < Nz; ++k)
    {
        for (int j = 0; j < Ny; ++j)
        {
            for (int i = 0; i < Nx; ++i)
            {
                int cIdx = cellIndex(i, j, k);

                int p000 = pointIndex(i    , j    , k    );
                int p100 = pointIndex(i + 1, j    , k    );
                int p010 = pointIndex(i    , j + 1, k    );
                int p110 = pointIndex(i + 1, j + 1, k    );
                int p001 = pointIndex(i    , j    , k + 1);
                int p101 = pointIndex(i + 1, j    , k + 1);
                int p011 = pointIndex(i    , j + 1, k + 1);
                int p111 = pointIndex(i + 1, j + 1, k + 1);

                const Point& a  = pts[p000];
                const Point& b  = pts[p100];
                const Point& c1 = pts[p010];
                const Point& d  = pts[p110];
                const Point& e  = pts[p001];
                const Point& f  = pts[p101];
                const Point& g  = pts[p011];
                const Point& h  = pts[p111];

                Point c{};
                c.x = (a.x + b.x + c1.x + d.x + e.x + f.x + g.x + h.x) / 8.0;
                c.y = (a.y + b.y + c1.y + d.y + e.y + f.y + g.y + h.y) / 8.0;
                c.z = (a.z + b.z + c1.z + d.z + e.z + f.z + g.z + h.z) / 8.0;

                if (inDomain(c))
                {
                    keepCell[cIdx] = 1;
                    ++newCellCount;
                }
            }
        }
    }

    if (newCellCount == 0)
    {
        std::cerr << "applyMask: no cells left after masking!\n";
        std::exit(1);
    }

    // 旧 cell -> 新 cell 的映射（压缩编号）
    std::vector<int> cellMap(nCellsOld, -1);
    {
        int curIdx = 0;
        for (int c = 0; c < nCellsOld; ++c)
        {
            if (keepCell[c])
            {
                cellMap[c] = curIdx++;
            }
        }
    }

    // 2) 用保留下来的 cell 重建 faces / owner / neighbour
    using FaceKey = std::array<int,4>;

    struct TmpFace
    {
        std::array<int,4> verts;
        int owner = -1;
        int neighbour = -1;  // -1 表示 boundary face
    };

    std::vector<TmpFace> tmpFaces;
    std::map<FaceKey, int> faceMap;  // key: 排序后的 4 点索引 -> face index

    auto addFace = [&](int v0, int v1, int v2, int v3, int cellNew)
    {
        // 按“owner 的朝外法向”顺序保存几何
        TmpFace f;
        f.verts = {v0, v1, v2, v3};

        // 做一个排序后的 key 用来识别“同一个几何面”
        FaceKey key = {v0, v1, v2, v3};
        std::sort(key.begin(), key.end());

        auto it = faceMap.find(key);
        if (it == faceMap.end())
        {
            // 第一次见到：作为 owner face
            int idx = static_cast<int>(tmpFaces.size());
            faceMap[key] = idx;
            f.owner = cellNew;
            f.neighbour = -1;
            tmpFaces.push_back(f);
        }
        else
        {
            // 第二次见到：作为 neighbour
            int idx = it->second;
            if (tmpFaces[idx].neighbour != -1)
            {
                std::cerr << "applyMask: non-manifold face detected (more than 2 cells share one face)\n";
                std::exit(1);
            }
            tmpFaces[idx].neighbour = cellNew;
        }
    };

    // 2.1 扫描所有保留的 cell，生成 6 个面
    for (int k = 0; k < Nz; ++k)
    {
        for (int j = 0; j < Ny; ++j)
        {
            for (int i = 0; i < Nx; ++i)
            {
                int cOld = cellIndex(i, j, k);
                if (!keepCell[cOld]) continue;

                int cNew = cellMap[cOld];

                int p000 = pointIndex(i    , j    , k    );
                int p100 = pointIndex(i + 1, j    , k    );
                int p010 = pointIndex(i    , j + 1, k    );
                int p110 = pointIndex(i + 1, j + 1, k    );
                int p001 = pointIndex(i    , j    , k + 1);
                int p101 = pointIndex(i + 1, j    , k + 1);
                int p011 = pointIndex(i    , j + 1, k + 1);
                int p111 = pointIndex(i + 1, j + 1, k + 1);

                // 统一使用“对于该 cell，法向指向外侧”的顶点顺序
                // 以 cell 盒子 [x0,x1]x[y0,y1]x[z0,z1] 为例：
                //  xmin: normal 指向 -x
                //  xmax: normal 指向 +x
                //  ymin: normal 指向 -y
                //  ymax: normal 指向 +y
                //  zmin: normal 指向 -z
                //  zmax: normal 指向 +z

                // xmin 面 (x = x0)：[p000, p001, p011, p010]
                addFace(p000, p001, p011, p010, cNew);

                // xmax 面 (x = x1)：[p100, p110, p111, p101]
                addFace(p100, p110, p111, p101, cNew);

                // ymin 面 (y = y0)：[p000, p100, p101, p001]
                addFace(p000, p100, p101, p001, cNew);

                // ymax 面 (y = y1)：[p010, p011, p111, p110]
                addFace(p010, p011, p111, p110, cNew);

                // zmin 面 (z = z0)：[p000, p010, p110, p100]
                addFace(p000, p010, p110, p100, cNew);

                // zmax 面 (z = z1)：[p001, p101, p111, p011]
                addFace(p001, p101, p111, p011, cNew);
            }
        }
    }

    // 3) 把 tmpFaces 拆成 internal + boundary
    MeshData out;
    out.Nx = Nx;
    out.Ny = Ny;
    out.Nz = Nz;
    out.points = bgMesh.points;  // 先保留所有点（unused points 可以以后再清）

    out.faces.clear();
    out.owner.clear();
    out.neighbour.clear();

    out.faces.reserve(tmpFaces.size());
    out.owner.reserve(tmpFaces.size());

    // 3.1 先写 internal faces（neighbour != -1）
    for (const auto& f : tmpFaces)
    {
        if (f.neighbour != -1)
        {
            out.faces.push_back(f.verts);
            out.owner.push_back(f.owner);
            out.neighbour.push_back(f.neighbour);
        }
    }

    const int nInternalFacesNew = static_cast<int>(out.faces.size());

    // 4) boundary faces：neighbour == -1 → 按坐标划分 patch
    double xmin =  1e30, xmax = -1e30;
    double ymin =  1e30, ymax = -1e30;
    double zmin =  1e30, zmax = -1e30;

    for (const auto& p : out.points)
    {
        if (p.x < xmin) xmin = p.x;
        if (p.x > xmax) xmax = p.x;
        if (p.y < ymin) ymin = p.y;
        if (p.y > ymax) ymax = p.y;
        if (p.z < zmin) zmin = p.z;
        if (p.z > zmax) zmax = p.z;
    }

    double Lx = xmax - xmin;
    double Ly = ymax - ymin;
    double Lz = zmax - zmin;
    double L  = std::max(Lx, std::max(Ly, Lz));
    if (L <= 0.0) L = 1.0;
    double tol = 1e-8 * L;

    std::vector<std::array<int,4>> facesBack, facesFront, facesBottom, facesTop, facesReflector, facesLeft;
    std::vector<int> ownerBack, ownerFront, ownerBottom, ownerTop, ownerReflector, ownerLeft;

    auto classifyFace = [&](const std::array<int,4>& fPts, int own)
    {
        Point fc{};
        for (int k = 0; k < 4; ++k)
        {
            const auto& pt = pts[fPts[k]];
            fc.x += pt.x;
            fc.y += pt.y;
            fc.z += pt.z;
        }
        fc.x *= 0.25;
        fc.y *= 0.25;
        fc.z *= 0.25;

        bool onZmin = std::fabs(fc.z - zmin) <= tol;
        bool onZmax = std::fabs(fc.z - zmax) <= tol;
        bool onYmin = std::fabs(fc.y - ymin) <= tol;
        bool onYmax = std::fabs(fc.y - ymax) <= tol;
        bool onXmin = std::fabs(fc.x - xmin) <= tol;

        if (onZmin)
        {
            facesBack.push_back(fPts);
            ownerBack.push_back(own);
        }
        else if (onZmax)
        {
            facesFront.push_back(fPts);
            ownerFront.push_back(own);
        }
        else if (onYmin)
        {
            facesBottom.push_back(fPts);
            ownerBottom.push_back(own);
        }
        else if (onYmax)
        {
            facesTop.push_back(fPts);
            ownerTop.push_back(own);
        }
        else if (onXmin)
        {
            facesLeft.push_back(fPts);
            ownerLeft.push_back(own);
        }
        else
        {
            facesReflector.push_back(fPts);
            ownerReflector.push_back(own);
        }
    };

    for (const auto& f : tmpFaces)
    {
        if (f.neighbour == -1)
        {
            classifyFace(f.verts, f.owner);
        }
    }

    // 5) 追加 boundary faces（顺序：back -> front -> bottom -> top -> reflector -> left）
    int faceStart = nInternalFacesNew;

    // back
    out.startFaceBack = faceStart;
    out.nFacesBack    = static_cast<int>(facesBack.size());
    for (size_t i = 0; i < facesBack.size(); ++i)
    {
        out.faces.push_back(facesBack[i]);
        out.owner.push_back(ownerBack[i]);
    }
    faceStart += out.nFacesBack;

    // front
    out.startFaceFront = faceStart;
    out.nFacesFront    = static_cast<int>(facesFront.size());
    for (size_t i = 0; i < facesFront.size(); ++i)
    {
        out.faces.push_back(facesFront[i]);
        out.owner.push_back(ownerFront[i]);
    }
    faceStart += out.nFacesFront;

    // bottom
    out.startFaceBottom = faceStart;
    out.nFacesBottom    = static_cast<int>(facesBottom.size());
    for (size_t i = 0; i < facesBottom.size(); ++i)
    {
        out.faces.push_back(facesBottom[i]);
        out.owner.push_back(ownerBottom[i]);
    }
    faceStart += out.nFacesBottom;

    // top
    out.startFaceTop = faceStart;
    out.nFacesTop    = static_cast<int>(facesTop.size());
    for (size_t i = 0; i < facesTop.size(); ++i)
    {
        out.faces.push_back(facesTop[i]);
        out.owner.push_back(ownerTop[i]);
    }
    faceStart += out.nFacesTop;

    // reflector -> 用 Right 字段
    out.startFaceRight = faceStart;
    out.nFacesRight    = static_cast<int>(facesReflector.size());
    for (size_t i = 0; i < facesReflector.size(); ++i)
    {
        out.faces.push_back(facesReflector[i]);
        out.owner.push_back(ownerReflector[i]);
    }
    faceStart += out.nFacesRight;

    // left
    out.startFaceLeft = faceStart;
    out.nFacesLeft    = static_cast<int>(facesLeft.size());
    for (size_t i = 0; i < facesLeft.size(); ++i)
    {
        out.faces.push_back(facesLeft[i]);
        out.owner.push_back(ownerLeft[i]);
    }
    faceStart += out.nFacesLeft;

    // neighbour 只对应 internal faces
    if (static_cast<int>(out.neighbour.size()) != nInternalFacesNew)
    {
        std::cerr << "applyMask: neighbour size mismatch\n";
        std::exit(1);
    }

    if (out.faces.size() != out.owner.size())
    {
        std::cerr << "applyMask: faces/owner size mismatch\n";
        std::exit(1);
    }

    std::cout << "applyMask: old cells = " << nCellsOld
              << ", new cells = " << newCellCount << "\n";
    std::cout << "applyMask: internal faces new = " << nInternalFacesNew
              << ", boundary faces = "
              << (out.faces.size() - nInternalFacesNew) << "\n";

    return out;
}
