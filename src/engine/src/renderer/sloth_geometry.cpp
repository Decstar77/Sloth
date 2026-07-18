#include "sloth_geometry.h"

#include <glm/gtc/constants.hpp>

#include <cmath>
#include <unordered_map>

namespace sloth
{
    MeshData Geometry::CreatePlane(f32 width, f32 depth, u32 widthSegments, u32 depthSegments, const glm::vec3& color)
    {
        widthSegments = widthSegments < 1 ? 1 : widthSegments;
        depthSegments = depthSegments < 1 ? 1 : depthSegments;

        MeshData mesh;

        f32 halfWidth = width * 0.5f;
        f32 halfDepth = depth * 0.5f;
        u32 columns = depthSegments + 1;

        for (u32 ix = 0; ix <= widthSegments; ++ix)
        {
            f32 x = -halfWidth + width * (static_cast<f32>(ix) / static_cast<f32>(widthSegments));
            for (u32 iz = 0; iz <= depthSegments; ++iz)
            {
                f32 z = -halfDepth + depth * (static_cast<f32>(iz) / static_cast<f32>(depthSegments));
                mesh.Vertices.push_back({ glm::vec3(x, 0.0f, z), color });
            }
        }

        for (u32 ix = 0; ix < widthSegments; ++ix)
        {
            for (u32 iz = 0; iz < depthSegments; ++iz)
            {
                u32 i00 = ix * columns + iz;
                u32 i10 = (ix + 1) * columns + iz;
                u32 i11 = (ix + 1) * columns + (iz + 1);
                u32 i01 = ix * columns + (iz + 1);

                mesh.Indices.insert(mesh.Indices.end(), { i00, i11, i10, i00, i01, i11 });
            }
        }

        return mesh;
    }

    MeshData Geometry::CreateBox(f32 width, f32 height, f32 depth, const glm::vec3& color)
    {
        MeshData mesh;

        f32 hw = width * 0.5f;
        f32 hh = height * 0.5f;
        f32 hd = depth * 0.5f;

        auto AddQuad = [&](const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3)
        {
            u32 baseIndex = static_cast<u32>(mesh.Vertices.size());
            mesh.Vertices.insert(mesh.Vertices.end(), { { v0, color }, { v1, color }, { v2, color }, { v3, color } });
            mesh.Indices.insert(mesh.Indices.end(),
                                 { baseIndex, baseIndex + 1, baseIndex + 2, baseIndex, baseIndex + 2, baseIndex + 3 });
        };

        glm::vec3 p000(-hw, -hh, -hd), p100(hw, -hh, -hd), p110(hw, hh, -hd), p010(-hw, hh, -hd);
        glm::vec3 p001(-hw, -hh, hd), p101(hw, -hh, hd), p111(hw, hh, hd), p011(-hw, hh, hd);

        AddQuad(p001, p101, p111, p011); // +Z front
        AddQuad(p100, p000, p010, p110); // -Z back
        AddQuad(p101, p100, p110, p111); // +X right
        AddQuad(p000, p001, p011, p010); // -X left
        AddQuad(p011, p111, p110, p010); // +Y top
        AddQuad(p000, p100, p101, p001); // -Y bottom

        return mesh;
    }

    MeshData Geometry::CreateUVSphere(f32 radius, u32 latitudeSegments, u32 longitudeSegments, const glm::vec3& color)
    {
        latitudeSegments = latitudeSegments < 2 ? 2 : latitudeSegments;
        longitudeSegments = longitudeSegments < 3 ? 3 : longitudeSegments;

        MeshData mesh;

        for (u32 lat = 0; lat <= latitudeSegments; ++lat)
        {
            f32 phi = glm::pi<f32>() * static_cast<f32>(lat) / static_cast<f32>(latitudeSegments);
            f32 y = radius * std::cos(phi);
            f32 ringRadius = radius * std::sin(phi);

            for (u32 lon = 0; lon <= longitudeSegments; ++lon)
            {
                f32 theta = 2.0f * glm::pi<f32>() * static_cast<f32>(lon) / static_cast<f32>(longitudeSegments);
                f32 x = ringRadius * std::cos(theta);
                f32 z = ringRadius * std::sin(theta);

                mesh.Vertices.push_back({ glm::vec3(x, y, z), color });
            }
        }

        // Skip degenerate triangles at the poles (Song Ho Ahn's UV sphere layout).
        for (u32 lat = 0; lat < latitudeSegments; ++lat)
        {
            u32 k1 = lat * (longitudeSegments + 1);
            u32 k2 = k1 + longitudeSegments + 1;

            for (u32 lon = 0; lon < longitudeSegments; ++lon, ++k1, ++k2)
            {
                if (lat != 0)
                {
                    mesh.Indices.insert(mesh.Indices.end(), { k1, k2, k1 + 1 });
                }

                if (lat != latitudeSegments - 1)
                {
                    mesh.Indices.insert(mesh.Indices.end(), { k1 + 1, k2, k2 + 1 });
                }
            }
        }

        return mesh;
    }

    MeshData Geometry::CreateIsoSphere(f32 radius, u32 subdivisions, const glm::vec3& color)
    {
        std::vector<glm::vec3> positions;
        std::vector<u32> triangles;

        f32 t = (1.0f + std::sqrt(5.0f)) * 0.5f;

        positions = {
            glm::normalize(glm::vec3(-1, t, 0)),  glm::normalize(glm::vec3(1, t, 0)),
            glm::normalize(glm::vec3(-1, -t, 0)), glm::normalize(glm::vec3(1, -t, 0)),
            glm::normalize(glm::vec3(0, -1, t)),  glm::normalize(glm::vec3(0, 1, t)),
            glm::normalize(glm::vec3(0, -1, -t)), glm::normalize(glm::vec3(0, 1, -t)),
            glm::normalize(glm::vec3(t, 0, -1)),  glm::normalize(glm::vec3(t, 0, 1)),
            glm::normalize(glm::vec3(-t, 0, -1)), glm::normalize(glm::vec3(-t, 0, 1)),
        };

        triangles = {
            0, 11, 5, 0, 5, 1,  0, 1, 7,  0, 7, 10, 0, 10, 11,
            1, 5, 9, 5, 11, 4,  11, 10, 2, 10, 7, 6, 7, 1, 8,
            3, 9, 4, 3, 4, 2,   3, 2, 6,  3, 6, 8,  3, 8, 9,
            4, 9, 5, 2, 4, 11,  6, 2, 10, 8, 6, 7,  9, 8, 1,
        };

        std::unordered_map<u64, u32> midpointCache;
        auto GetMidpoint = [&](u32 i0, u32 i1) -> u32
        {
            u64 smaller = i0 < i1 ? i0 : i1;
            u64 greater = i0 < i1 ? i1 : i0;
            u64 key = (smaller << 32) | greater;

            auto it = midpointCache.find(key);
            if (it != midpointCache.end())
            {
                return it->second;
            }

            glm::vec3 mid = glm::normalize((positions[i0] + positions[i1]) * 0.5f);
            positions.push_back(mid);
            u32 index = static_cast<u32>(positions.size() - 1);
            midpointCache[key] = index;
            return index;
        };

        for (u32 level = 0; level < subdivisions; ++level)
        {
            std::vector<u32> subdivided;
            subdivided.reserve(triangles.size() * 4);

            for (usize i = 0; i < triangles.size(); i += 3)
            {
                u32 v0 = triangles[i + 0];
                u32 v1 = triangles[i + 1];
                u32 v2 = triangles[i + 2];

                u32 a = GetMidpoint(v0, v1);
                u32 b = GetMidpoint(v1, v2);
                u32 c = GetMidpoint(v2, v0);

                subdivided.insert(subdivided.end(), { v0, a, c, v1, b, a, v2, c, b, a, b, c });
            }

            triangles = std::move(subdivided);
            midpointCache.clear();
        }

        MeshData mesh;
        mesh.Vertices.reserve(positions.size());
        for (const glm::vec3& position : positions)
        {
            mesh.Vertices.push_back({ position * radius, color });
        }
        mesh.Indices = std::move(triangles);

        return mesh;
    }

    MeshData Geometry::CreateCylinder(f32 radius, f32 height, u32 radialSegments, const glm::vec3& color)
    {
        radialSegments = radialSegments < 3 ? 3 : radialSegments;

        MeshData mesh;

        f32 halfHeight = height * 0.5f;

        auto RingPosition = [&](u32 segment, f32 y)
        {
            f32 theta = 2.0f * glm::pi<f32>() * static_cast<f32>(segment) / static_cast<f32>(radialSegments);
            return glm::vec3(radius * std::cos(theta), y, radius * std::sin(theta));
        };

        // Side surface: two rings, seam vertex duplicated so the UV/color
        // parameterization can wrap cleanly if ever needed.
        u32 topRingStart = 0;
        for (u32 s = 0; s <= radialSegments; ++s)
        {
            mesh.Vertices.push_back({ RingPosition(s, halfHeight), color });
        }

        u32 bottomRingStart = static_cast<u32>(mesh.Vertices.size());
        for (u32 s = 0; s <= radialSegments; ++s)
        {
            mesh.Vertices.push_back({ RingPosition(s, -halfHeight), color });
        }

        for (u32 s = 0; s < radialSegments; ++s)
        {
            u32 top0 = topRingStart + s;
            u32 top1 = topRingStart + s + 1;
            u32 bottom0 = bottomRingStart + s;
            u32 bottom1 = bottomRingStart + s + 1;

            mesh.Indices.insert(mesh.Indices.end(), { top0, top1, bottom0, top1, bottom1, bottom0 });
        }

        // Top cap (outward normal +Y).
        u32 topCenterIndex = static_cast<u32>(mesh.Vertices.size());
        mesh.Vertices.push_back({ glm::vec3(0.0f, halfHeight, 0.0f), color });
        u32 topCapRingStart = static_cast<u32>(mesh.Vertices.size());
        for (u32 s = 0; s <= radialSegments; ++s)
        {
            mesh.Vertices.push_back({ RingPosition(s, halfHeight), color });
        }
        for (u32 s = 0; s < radialSegments; ++s)
        {
            mesh.Indices.insert(mesh.Indices.end(), { topCenterIndex, topCapRingStart + s + 1, topCapRingStart + s });
        }

        // Bottom cap (outward normal -Y).
        u32 bottomCenterIndex = static_cast<u32>(mesh.Vertices.size());
        mesh.Vertices.push_back({ glm::vec3(0.0f, -halfHeight, 0.0f), color });
        u32 bottomCapRingStart = static_cast<u32>(mesh.Vertices.size());
        for (u32 s = 0; s <= radialSegments; ++s)
        {
            mesh.Vertices.push_back({ RingPosition(s, -halfHeight), color });
        }
        for (u32 s = 0; s < radialSegments; ++s)
        {
            mesh.Indices.insert(mesh.Indices.end(),
                                 { bottomCenterIndex, bottomCapRingStart + s, bottomCapRingStart + s + 1 });
        }

        return mesh;
    }

} // namespace sloth
