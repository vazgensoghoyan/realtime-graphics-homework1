#pragma once

#include <obj_loader.hpp>

#include <cstdint>

// Unit sphere centered at the origin, with Y as the polar axis.
// sectors >= 3 longitude segments; stacks >= 2 pole-to-pole segments.
// Vertices are shared at the seam and poles; texture coordinates are zero.
ObjMesh generateSphere(uint32_t sectors, uint32_t stacks);
