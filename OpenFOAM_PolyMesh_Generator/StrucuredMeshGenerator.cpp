
#include <iostream>
#include <cstdlib>
#include <cmath>
#include "StructuredMeshGenerator.h"

MeshData generateStructuredMesh(int Nx, int Ny, int Nz,
                                double Lx, double Ly, double Lz)
{
    MeshData m;
    m.Nx = Nx; m.Ny = Ny; m.Nz = Nz;

    const int nPoints = (Nx+1)*(Ny+1)*(Nz+1);
    m.points.resize(nPoints);

    auto pointIndex = [Nx, Ny](int i, int j, int k) -> int
    {
        return k * (Ny + 1) * (Nx + 1) + j * (Nx + 1) + i;
    };

    // 1) 生成点坐标
    double dx = Lx / Nx;
    double dy = Ly / Ny;
    double dz = Lz / Nz;

    for (int k = 0; k <= Nz; ++k)
    {
        for (int j = 0; j <= Ny; ++j)
        {
            for (int i = 0; i <= Nx; ++i)
            {
                int p = pointIndex(i,j,k);
                m.points[p].x = i * dx;
                m.points[p].y = j * dy;
                m.points[p].z = k * dz;
            }
        }
    }

    // 2) 生成 internal faces
    std::vector<std::array<int,4>> faces;
    std::vector<int> owner;
    std::vector<int> neighbour;

    faces.reserve(3 * Nx * Ny * Nz);  // 粗略预估
    owner.reserve(3 * Nx * Ny * Nz);
    neighbour.reserve(3 * Nx * Ny * Nz);

    auto cellIndex = [Nx, Ny](int i, int j, int k) -> int
    {
        return k * (Ny * Nx) + j * Nx + i;
    };

    // 2.1 x 方向 internal faces, 法向 ~ +x
    for (int k = 0; k < Nz; ++k)
    {
        for (int j = 0; j < Ny; ++j)
        {
            for (int i = 0; i < Nx-1; ++i)
            {
                int cL = cellIndex(i,   j, k); // owner
                int cR = cellIndex(i+1, j, k); // neighbour

                int iFace = i+1;
                int j0 = j;
                int j1 = j+1;
                int k0 = k;
                int k1 = k+1;

                std::array<int,4> f;
                f[0] = pointIndex(iFace, j0, k0);
                f[1] = pointIndex(iFace, j1, k0);
                f[2] = pointIndex(iFace, j1, k1);
                f[3] = pointIndex(iFace, j0, k1);

                faces.push_back(f);
                owner.push_back(cL);
                neighbour.push_back(cR);
            }
        }
    }

    // 2.2 y 方向 internal faces, 法向 ~ +y
    for (int k = 0; k < Nz; ++k)
    {
        for (int j = 0; j < Ny-1; ++j)
        {
            for (int i = 0; i < Nx; ++i)
            {
                int cD = cellIndex(i, j,   k); // owner (down)
                int cU = cellIndex(i, j+1, k); // neighbour (up)

                int jFace = j+1;
                int i0 = i;
                int i1 = i+1;
                int k0 = k;
                int k1 = k+1;

                std::array<int,4> f;
                f[0] = pointIndex(i0, jFace, k0);
                f[1] = pointIndex(i1, jFace, k0);
                f[2] = pointIndex(i1, jFace, k1);
                f[3] = pointIndex(i0, jFace, k1);

                faces.push_back(f);
                owner.push_back(cD);
                neighbour.push_back(cU);
            }
        }
    }

    // 2.3 z 方向 internal faces, 法向 ~ +z
    for (int k = 0; k < Nz-1; ++k)
    {
        for (int j = 0; j < Ny; ++j)
        {
            for (int i = 0; i < Nx; ++i)
            {
                int cB = cellIndex(i, j, k);   // owner (bottom)
                int cT = cellIndex(i, j, k+1); // neighbour (top)

                int kFace = k+1;
                int i0 = i;
                int i1 = i+1;
                int j0 = j;
                int j1 = j+1;

                std::array<int,4> f;
                f[0] = pointIndex(i0, j0, kFace);
                f[1] = pointIndex(i1, j0, kFace);
                f[2] = pointIndex(i1, j1, kFace);
                f[3] = pointIndex(i0, j1, kFace);

                faces.push_back(f);
                owner.push_back(cB);
                neighbour.push_back(cT);
            }
        }
    }

    //  const int nInternalFaces = static_cast<int>(faces.size());

    // 3) boundary faces：只写 faces/owner，neighbour 不写
    //    这里先把 boundary faces 都当作“无 patch 信息”的，DomainMask 后面会重划 patch

    // 3.1 x = 0 (left 外壁)
    for (int k = 0; k < Nz; ++k)
    {
        for (int j = 0; j < Ny; ++j)
        {
            int c = cellIndex(0, j, k); // 最左一列 cell

            int iFace = 0;
            int j0 = j;
            int j1 = j+1;
            int k0 = k;
            int k1 = k+1;

            std::array<int,4> f;
            f[0] = pointIndex(iFace, j0, k0);
            f[1] = pointIndex(iFace, j0, k1);
            f[2] = pointIndex(iFace, j1, k1);
            f[3] = pointIndex(iFace, j1, k0);

            faces.push_back(f);
            owner.push_back(c);
            // 不 push neighbour
        }
    }

    // 3.2 x = Lx (right 外壁)
    for (int k = 0; k < Nz; ++k)
    {
        for (int j = 0; j < Ny; ++j)
        {
            int c = cellIndex(Nx-1, j, k); // 最右一列 cell

            int iFace = Nx;
            int j0 = j;
            int j1 = j+1;
            int k0 = k;
            int k1 = k+1;

            std::array<int,4> f;
            f[0] = pointIndex(iFace, j0, k0);
            f[1] = pointIndex(iFace, j1, k0);
            f[2] = pointIndex(iFace, j1, k1);
            f[3] = pointIndex(iFace, j0, k1);

            faces.push_back(f);
            owner.push_back(c);
        }
    }

    // 3.3 y = 0 (bottom 外壁)
    for (int k = 0; k < Nz; ++k)
    {
        for (int i = 0; i < Nx; ++i)
        {
            int c = cellIndex(i, 0, k);

            int jFace = 0;
            int i0 = i;
            int i1 = i+1;
            int k0 = k;
            int k1 = k+1;

            std::array<int,4> f;
            f[0] = pointIndex(i0, jFace, k0);
            f[1] = pointIndex(i1, jFace, k0);
            f[2] = pointIndex(i1, jFace, k1);
            f[3] = pointIndex(i0, jFace, k1);

            faces.push_back(f);
            owner.push_back(c);
        }
    }

    // 3.4 y = Ly (top 外壁)
    for (int k = 0; k < Nz; ++k)
    {
        for (int i = 0; i < Nx; ++i)
        {
            int c = cellIndex(i, Ny-1, k);

            int jFace = Ny;
            int i0 = i;
            int i1 = i+1;
            int k0 = k;
            int k1 = k+1;

            std::array<int,4> f;
            f[0] = pointIndex(i0, jFace, k0);
            f[1] = pointIndex(i0, jFace, k1);
            f[2] = pointIndex(i1, jFace, k1);
            f[3] = pointIndex(i1, jFace, k0);

            faces.push_back(f);
            owner.push_back(c);
        }
    }

    // 3.5 z = 0 (back 外壁)
    for (int j = 0; j < Ny; ++j)
    {
        for (int i = 0; i < Nx; ++i)
        {
            int c = cellIndex(i, j, 0);

            int kFace = 0;
            int i0 = i;
            int i1 = i+1;
            int j0 = j;
            int j1 = j+1;

            std::array<int,4> f;
            f[0] = pointIndex(i0, j0, kFace);
            f[1] = pointIndex(i0, j1, kFace);
            f[2] = pointIndex(i1, j1, kFace);
            f[3] = pointIndex(i1, j0, kFace);

            faces.push_back(f);
            owner.push_back(c);
        }
    }

    // 3.6 z = Lz (front 外壁)
    for (int j = 0; j < Ny; ++j)
    {
        for (int i = 0; i < Nx; ++i)
        {
            int c = cellIndex(i, j, Nz-1);

            int kFace = Nz;
            int i0 = i;
            int i1 = i+1;
            int j0 = j;
            int j1 = j+1;

            std::array<int,4> f;
            f[0] = pointIndex(i0, j0, kFace);
            f[1] = pointIndex(i1, j0, kFace);
            f[2] = pointIndex(i1, j1, kFace);
            f[3] = pointIndex(i0, j1, kFace);

            faces.push_back(f);
            owner.push_back(c);
        }
    }

    // ==== 收尾 ====
    m.faces     = std::move(faces);
    m.owner     = std::move(owner);
    m.neighbour = std::move(neighbour);

    // patch 起点留空，交给后面的 DomainMask 重新划分
    m.startFaceFront = m.startFaceBack = m.startFaceTop = m.startFaceBottom =
    m.startFaceLeft  = m.startFaceRight = 0;
    m.nFacesFront = m.nFacesBack = m.nFacesTop = m.nFacesBottom =
    m.nFacesLeft  = m.nFacesRight = 0;

    return m;
}
