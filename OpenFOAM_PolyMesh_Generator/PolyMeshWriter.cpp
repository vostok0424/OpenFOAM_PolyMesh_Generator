
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include "PolyMeshWriter.h"

void writePolyMesh(const MeshData &mesh, const std::string &baseDir)
{
    std::filesystem::create_directories(baseDir);

    // ---- points ----
    {
        std::ofstream out(baseDir + "/points");
        if (!out)
        {
            std::cerr << "Cannot open points file for writing.\n";
            std::exit(1);
        }

        out <<
"FoamFile\n"
"{\n"
"    version     2.0;\n"
"    format      ascii;\n"
"    class       vectorField;\n"
"    location    \"polyMesh\";\n"
"    object      points;\n"
"}\n"
"\n"
"// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n"
"\n";

        out << mesh.points.size() << "\n(\n";
        for (const auto &p : mesh.points)
        {
            out << "(" << p.x << " " << p.y << " " << p.z << ")\n";
        }
        out << ")\n;\n\n";
    }

    // ---- faces ----
    {
        std::ofstream out(baseDir + "/faces");
        if (!out)
        {
            std::cerr << "Cannot open faces file for writing.\n";
            std::exit(1);
        }

        out <<
"FoamFile\n"
"{\n"
"    version     2.0;\n"
"    format      ascii;\n"
"    class       faceList;\n"
"    location    \"polyMesh\";\n"
"    object      faces;\n"
"}\n"
"\n"
"// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n"
"\n";

        out << mesh.faces.size() << "\n(\n";
        for (const auto &f : mesh.faces)
        {
            out << "4(" << f[0] << " " << f[1] << " "
                        << f[2] << " " << f[3] << ")\n";
        }
        out << ")\n;\n\n";
    }

    // ---- owner ----
    {
        std::ofstream out(baseDir + "/owner");
        if (!out)
        {
            std::cerr << "Cannot open owner file for writing.\n";
            std::exit(1);
        }

        out <<
"FoamFile\n"
"{\n"
"    version     2.0;\n"
"    format      ascii;\n"
"    class       labelList;\n"
"    location    \"polyMesh\";\n"
"    object      owner;\n"
"}\n"
"\n"
"// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n"
"\n";

        out << mesh.owner.size() << "\n(\n";
        for (int c : mesh.owner)
        {
            out << c << "\n";
        }
        out << ")\n;\n\n";
    }

    // ---- neighbour ----
    {
        std::ofstream out(baseDir + "/neighbour");
        if (!out)
        {
            std::cerr << "Cannot open neighbour file for writing.\n";
            std::exit(1);
        }

        out <<
"FoamFile\n"
"{\n"
"    version     2.0;\n"
"    format      ascii;\n"
"    class       labelList;\n"
"    location    \"polyMesh\";\n"
"    object      neighbour;\n"
"}\n"
"\n"
"// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n"
"\n";

        out << mesh.neighbour.size() << "\n(\n";
        for (int c : mesh.neighbour)
        {
            out << c << "\n";
        }
        out << ")\n;\n\n";
    }

    // ---- boundary ----
    {
        std::ofstream out(baseDir + "/boundary");
        if (!out)
        {
            std::cerr << "Cannot open boundary file for writing.\n";
            std::exit(1);
        }

        out <<
"FoamFile\n"
"{\n"
"    version     2.0;\n"
"    format      ascii;\n"
"    class       polyBoundaryMesh;\n"
"    location    \"polyMesh\";\n"
"    object      boundary;\n"
"}\n"
"\n"
"// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n"
"\n";

        const int nPatches = 6;
        out << nPatches << "\n(\n";

        // left
        out <<
"left\n"
"{\n"
"    type            patch;\n"
"    physicalType    patch;\n"
"    nFaces          " << mesh.nFacesLeft << ";\n"
"    startFace       " << mesh.startFaceLeft << ";\n"
"}\n";

        // right
        out <<
"right\n"
"{\n"
"    type            patch;\n"
"    physicalType    patch;\n"
"    nFaces          " << mesh.nFacesRight << ";\n"
"    startFace       " << mesh.startFaceRight << ";\n"
"}\n";

        // bottom
        out <<
"bottom\n"
"{\n"
"    type            patch;\n"
"    physicalType    patch;\n"
"    nFaces          " << mesh.nFacesBottom << ";\n"
"    startFace       " << mesh.startFaceBottom << ";\n"
"}\n";

        // top
        out <<
"top\n"
"{\n"
"    type            patch;\n"
"    physicalType    patch;\n"
"    nFaces          " << mesh.nFacesTop << ";\n"
"    startFace       " << mesh.startFaceTop << ";\n"
"}\n";

        // front (z-min)
        out <<
"front\n"
"{\n"
"    type            patch;\n"
"    physicalType    patch;\n"
"    nFaces          " << mesh.nFacesFront << ";\n"
"    startFace       " << mesh.startFaceFront << ";\n"
"}\n";

        // back (z-max)
        out <<
"back\n"
"{\n"
"    type            patch;\n"
"    physicalType    patch;\n"
"    nFaces          " << mesh.nFacesBack << ";\n"
"    startFace       " << mesh.startFaceBack << ";\n"
"}\n";

        out << ")\n;\n\n";
    }

    std::cout << "polyMesh written to " << baseDir << "\n";
}
