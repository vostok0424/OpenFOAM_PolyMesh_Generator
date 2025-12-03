
#include "MeshCleaner.h"

#include <vector>
#include <iostream>

void removeUnusedPoints(MeshData& mesh)
{
    const std::size_t nPoints = mesh.points.size();
    const std::size_t nFaces  = mesh.faces.size();

    if (nPoints == 0 || nFaces == 0)
    {
        return;
    }

    // 1) 标记哪些点被 faces 使用
    std::vector<char> used(nPoints, 0);

    for (const auto& f : mesh.faces)
    {
        for (int k = 0; k < 4; ++k)
        {
            int pid = f[k];
            if (pid < 0 || static_cast<std::size_t>(pid) >= nPoints)
            {
                std::cerr << "removeUnusedPoints: face uses invalid point index "
                          << pid << " (nPoints=" << nPoints << ")\n";
                return; // 不继续操作，避免进一步破坏
            }
            used[pid] = 1;
        }
    }

    // 2) 建立 old -> new 的编号映射，只给被使用的点分配新编号
    std::vector<int> oldToNew(nPoints, -1);
    std::vector<Point> newPoints;
    newPoints.reserve(nPoints);

    int newIndex = 0;
    for (std::size_t i = 0; i < nPoints; ++i)
    {
        if (used[i])
        {
            oldToNew[i] = newIndex++;
            newPoints.push_back(mesh.points[i]);
        }
    }

    // 如果没有点被用到，就不要动
    if (newIndex == 0)
    {
        std::cerr << "removeUnusedPoints: no used points found, skip compaction.\n";
        return;
    }

    // 3) 用映射更新所有 faces 中的点索引
    for (auto& f : mesh.faces)
    {
        for (int k = 0; k < 4; ++k)
        {
            int oldId = f[k];
            int nid   = oldToNew[oldId];
            if (nid < 0)
            {
                std::cerr << "removeUnusedPoints: face references unused point "
                          << oldId << " after marking.\n";
                return;
            }
            f[k] = nid;
        }
    }

    // 4) 替换点数组
    mesh.points.swap(newPoints);

    std::cout << "removeUnusedPoints: compacted points from "
              << nPoints << " to " << mesh.points.size() << "\n";
}
