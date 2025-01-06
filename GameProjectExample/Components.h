#pragma once
#include "../Engine/__reflection.h"

// we define component name in code => cannot change component name in editor, but not scene, world name => can be edited by editor
COMPONENT_BEGIN(transform)
__SERIALIZE_FIELD(float3, position, 0, 0, 0)
__SERIALIZE_FIELD(float3, scale, 1, 1, 1)
__SERIALIZE_FIELD(float4, rotation, 0, 0, 0, 1)
COMPNENT_END()

COMPONENT_BEGIN(bullet)
__SERIALIZE_FIELD(float3, vel, 1, 0, 1)
__SERIALIZE_FIELD(float3, ac, 1, 1, 1)
COMPNENT_END()

COMPONENT_BEGIN(rigid_body)
COMPNENT_END()

// SCENE_BEGIN()
//
// SECENE(WORLD(transform,
//			 bullet,
//			 rigid_body),
//
//	   WORLD(transform,
//			 rigid_body),
//
//	   WORLD(transform,
//			 rigid_body,
//			 bullet))
//
// SCENE_END()
// auto scene_1 = ecs::scene<
//	ecs::world_descriptor<transform, bullet, rigid_body>,
//	ecs::world_descriptor<transform, rigid_body>,
//	ecs::world_descriptor<transform, rigid_body, bullet>>();