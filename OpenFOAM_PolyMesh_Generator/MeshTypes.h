
#pragma once

#include <vector>
#include <array>

// Simple 3D point
struct Point
{
    double x, y, z;
};

// Mesh data container
struct MeshData
{
    int Nx = 0;
    int Ny = 0;
    int Nz = 0;

    std::vector<Point> points;
    std::vector<std::array<int, 4>> faces;   // all faces: internal + boundary
    std::vector<int> owner;                  // size = nFaces
    std::vector<int> neighbour;              // size = nInternalFaces

    // Patch information (indices into faces/owner)
    int startFaceBack   = 0; int nFacesBack   = 0;
    int startFaceFront  = 0; int nFacesFront  = 0;
    int startFaceBottom = 0; int nFacesBottom = 0;
    int startFaceTop    = 0; int nFacesTop    = 0;
    int startFaceRight  = 0; int nFacesRight  = 0;
    int startFaceLeft   = 0; int nFacesLeft   = 0;
};
