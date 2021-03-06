/*
 * MxMeshGmshImporter.h
 *
 *  Created on: Jul 18, 2017
 *      Author: andy
 */

#ifndef SRC_MXMESHGMSHIMPORTER_H_
#define SRC_MXMESHGMSHIMPORTER_H_


#include "MxMesh.h"
#include <Magnum/Math/Vector3.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <GmshIO.h>

/**
 * Import a Gmsh mesh and generate a Mechanica mesh.
 *
 * The importer currently maps 3D Gmsh volume elements into Mechanica cells.
 *
 * Only 3D elements are supported, and all other elements are ignored. The
 * importer will determine the connectivity of all imported volume element
 * and connect them accordingly.
 */
class MxMeshGmshImporter {
public:

    MxMeshGmshImporter(MxMesh& mesh) : mesh{mesh} {};

    bool read(const std::string &path);

private:
    // keep track of original Gmsh vertex indices, and the new indices in
    // the MxMesh
    std::unordered_map<int, int> vertexMap;

    /**
     * A Gmsh node corresponds to a vertex (position, index)
     */
    uint addGmshVertex(const Gmsh::Node &node);

    void addCell(const Gmsh::Hexahedron &val);


    void addCell(const Gmsh::Prism &val);

    /**
     * Add a square face with the given vertices in CCW order.
     */
    void addSquareFace(MxCell &cell, const std::array<uint, 4> &verts);

    MxMesh &mesh;
    Gmsh::Mesh gmsh;
};

/* Node ordering for Gmsh Low order elements
   -----------------------------------------

For all mesh and post-processing file formats, the reference elements
are defined as follows.

     Line:                   Line3:           Line4:

     0----------1 --> u      0-----2----1     0----2----3----1


     Triangle:               Triangle6:          Triangle9/10:          Triangle12/15:

     v
     ^                                                                   2
     |                                                                   | \
     2                       2                    2                      9   8
     |`\                     |`\                  | \                    |     \
     |  `\                   |  `\                7   6                 10 (14)  7
     |    `\                 5    `4              |     \                |         \
     |      `\               |      `\            8  (9)  5             11 (12) (13) 6
     |        `\             |        `\          |         \            |             \
     0----------1 --> u      0-----3----1         0---3---4---1          0---3---4---5---1


     Quadrangle:            Quadrangle8:            Quadrangle9:

           v
           ^
           |
     3-----------2          3-----6-----2           3-----6-----2
     |     |     |          |           |           |           |
     |     |     |          |           |           |           |
     |     +---- | --> u    7           5           7     8     5
     |           |          |           |           |           |
     |           |          |           |           |           |
     0-----------1          0-----4-----1           0-----4-----1


     Tetrahedron:                          Tetrahedron10:

                        v
                      .
                    ,/
                   /
                2                                     2
              ,/|`\                                 ,/|`\
            ,/  |  `\                             ,/  |  `\
          ,/    '.   `\                         ,6    '.   `5
        ,/       |     `\                     ,/       8     `\
      ,/         |       `\                 ,/         |       `\
     0-----------'.--------1 --> u         0--------4--'.--------1
      `\.         |      ,/                 `\.         |      ,/
         `\.      |    ,/                      `\.      |    ,9
            `\.   '. ,/                           `7.   '. ,/
               `\. |/                                `\. |/
                  `3                                    `3
                     `\.
                        ` w

     Hexahedron:             Hexahedron20:          Hexahedron27:

            v
     3----------2            3----13----2           3----13----2
     |\     ^   |\           |\         |\          |\         |\
     | \    |   | \          | 15       | 14        |15    24  | 14
     |  \   |   |  \         9  \       11 \        9  \ 20    11 \
     |   7------+---6        |   7----19+---6       |   7----19+---6
     |   |  +-- |-- | -> u   |   |      |   |       |22 |  26  | 23|
     0---+---\--1   |        0---+-8----1   |       0---+-8----1   |
      \  |    \  \  |         \  17      \  18       \ 17    25 \  18
       \ |     \  \ |         10 |        12|        10 |  21    12|
        \|      w  \|           \|         \|          \|         \|
         4----------5            4----16----5           4----16----5


     Prism:                      Prism15:               Prism18:

                w
                ^
                |
                3                       3                      3
              ,/|`\                   ,/|`\                  ,/|`\
            ,/  |  `\               12  |  13              12  |  13
          ,/    |    `\           ,/    |    `\          ,/    |    `\
         4------+------5         4------14-----5        4------14-----5
         |      |      |         |      8      |        |      8      |
         |    ,/|`\    |         |      |      |        |    ,/|`\    |
         |  ,/  |  `\  |         |      |      |        |  15  |  16  |
         |,/    |    `\|         |      |      |        |,/    |    `\|
        ,|      |      |\        10     |      11       10-----17-----11
      ,/ |      0      | `\      |      0      |        |      0      |
     u   |    ,/ `\    |    v    |    ,/ `\    |        |    ,/ `\    |
         |  ,/     `\  |         |  ,6     `7  |        |  ,6     `7  |
         |,/         `\|         |,/         `\|        |,/         `\|
         1-------------2         1------9------2        1------9------2


     Pyramid:                     Pyramid13:                   Pyramid14:

                    4                            4                            4
                  ,/|\                         ,/|\                         ,/|\
                ,/ .'|\                      ,/ .'|\                      ,/ .'|\
              ,/   | | \                   ,/   | | \                   ,/   | | \
            ,/    .' | `.                ,/    .' | `.                ,/    .' | `.
          ,/      |  '.  \             ,7      |  12  \             ,7      |  12  \
        ,/       .' w |   \          ,/       .'   |   \          ,/       .'   |   \
      ,/         |  ^ |    \       ,/         9    |    11      ,/         9    |    11
     0----------.'--|-3    `.     0--------6-.'----3    `.     0--------6-.'----3    `.
      `\        |   |  `\    \      `\        |      `\    \     `\        |      `\    \
        `\     .'   +----`\ - \ -> v  `5     .'        10   \      `5     .' 13     10   \
          `\   |    `\     `\  \        `\   |           `\  \       `\   |           `\  \
            `\.'      `\     `\`          `\.'             `\`         `\.'             `\`
               1----------------2            1--------8-------2           1--------8-------2
                         `\
                            u

 ---------------------------------------------------------------------------------*/

#endif /* SRC_MXMESHGMSHIMPORTER_H_ */
