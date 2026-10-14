#pragma once

#include <obj_loader.hpp>

#include <cstdint>

ObjMesh generateRing(float innerRadius, float outerRadius, uint32_t segments = 64);
