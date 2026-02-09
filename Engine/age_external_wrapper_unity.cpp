#include "age_pch.hpp"
#include "age.hpp"

#include "external/include/mikktspace/mikktspace.h"
#include "external/include/meshoptimizer/meshoptimizer.h"
#include "external/include/earcut/earcut.hpp"

extern "C"
{
#include "external/include/mikktspace/mikktspace.c"
}

#include "age_external_wrapper_earcut.cpp"

namespace age::external::mikk
{
	bool
	gen_tangent(age::asset::mesh_editable& m, const float angular_threshold) noexcept
	{
		using namespace age::asset;
		auto mikkt_space_interface = SMikkTSpaceInterface{
			.m_getNumFaces = [](const SMikkTSpaceContext* p_ctx) {
				auto res = static_cast<mesh_editable*>(p_ctx->m_pUserData)->face_vec.size();
				return static_cast<int>(res); },

			.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* p_ctx, const int f_idx) {
				auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx->m_pUserData);
				return static_cast<int>( mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]).to_vertex_idx_count); },

			.m_getPosition	  = [](const SMikkTSpaceContext* p_ctx, float pos_out[], const int f_idx, const int outer_b2v_idx) {
				auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx->m_pUserData);
				auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
				auto& p = mesh_edit.position_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].pos_idx];
				std::memcpy(pos_out, &p, sizeof(p)); },
			.m_getNormal	  = [](const SMikkTSpaceContext* p_ctx, float normal_out[], const int f_idx, const int outer_b2v_idx) {
				auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx->m_pUserData);
				auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
				auto& v_attr = mesh_edit.vertex_attr_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].attribute_idx];
				std::memcpy(normal_out, &v_attr.normal, sizeof(v_attr.normal)); },
			.m_getTexCoord	  = [](const SMikkTSpaceContext* p_ctx, float uv_out[], const int f_idx, const int outer_b2v_idx) {
				auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx->m_pUserData);
				auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
				auto& v_attr = mesh_edit.vertex_attr_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].attribute_idx];
				std::memcpy(uv_out, &v_attr.uv_set[0], sizeof(v_attr.uv_set[0])); },
			.m_setTSpaceBasic = [](const SMikkTSpaceContext* p_ctx, const float tangent_in[], const float fsign, const int f_idx, const int outer_b2v_idx) {
				auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx->m_pUserData);
				auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
				auto& v_attr = mesh_edit.vertex_attr_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].attribute_idx];
		
				v_attr.tangent = { tangent_in[0], tangent_in[1], tangent_in[2], fsign }; }
		};

		auto ctx = SMikkTSpaceContext{
			.m_pInterface = &mikkt_space_interface,
			.m_pUserData  = static_cast<void*>(&m)
		};

		return genTangSpace(&ctx, angular_threshold);
	}
}	 // namespace age::external::mikk
