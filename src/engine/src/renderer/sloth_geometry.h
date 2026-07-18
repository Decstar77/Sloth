#pragma once

#include "core/sloth_defines.h"
#include "renderer/sloth_static_mesh.h"

#include <vector>

namespace sloth
{

    // CPU-side mesh data (position + color). Hand Vertices.data()/size() and
    // Indices.data()/size() to StaticMesh's constructor to upload it.
    struct MeshData
    {
        std::vector<Vertex> Vertices;
        std::vector<u32> Indices;
    };

    // Builder for common procedural primitives. Pure CPU-side geometry
    // generation with outward-facing CCW winding; does not touch the GPU.
    class Geometry
    {
    public:
        // Flat grid on the XZ plane, centered at the origin, facing +Y.
        static MeshData CreatePlane(f32 width, f32 depth, u32 widthSegments = 1, u32 depthSegments = 1,
                                    const glm::vec3& color = glm::vec3(1.0f));

        // Centered at the origin.
        static MeshData CreateBox(f32 width, f32 height, f32 depth, const glm::vec3& color = glm::vec3(1.0f));

        // Centered at the origin. Latitude/longitude ("UV") sphere; poles
        // pinch to a single point, so triangle density bunches up there.
        static MeshData CreateUVSphere(f32 radius, u32 latitudeSegments = 16, u32 longitudeSegments = 32,
                                        const glm::vec3& color = glm::vec3(1.0f));

        // Centered at the origin. Subdivided icosahedron; near-uniform
        // triangle distribution over the whole sphere, no pinched poles.
        // subdivisions == 0 yields the base 20-triangle icosahedron.
        static MeshData CreateIsoSphere(f32 radius, u32 subdivisions = 2, const glm::vec3& color = glm::vec3(1.0f));

        // Centered at the origin, axis along Y, with flat end caps.
        static MeshData CreateCylinder(f32 radius, f32 height, u32 radialSegments = 16,
                                        const glm::vec3& color = glm::vec3(1.0f));
    };

} // namespace sloth
