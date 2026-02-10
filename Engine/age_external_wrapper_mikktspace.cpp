#include "age_pch.hpp"
#include "age.hpp"

namespace age::external::mikk
{
	FORCE_INLINE bool
	gen_tangent(age::asset::mesh_editable& m, const float angular_threshold) noexcept
	{
		using namespace age::asset;

		return detail::gen_tangent_space(
			[](void* p_ctx) {
				auto res = static_cast<const mesh_editable*>(p_ctx)->face_vec.size();
				return static_cast<int>(res); },
			[](void* p_ctx, const int f_idx) {
				auto& mesh_edit = *static_cast<const mesh_editable*>(p_ctx);
				return static_cast<int>( mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]).to_vertex_idx_count); },

			[](void* p_ctx, float pos_out[], const int f_idx, const int outer_b2v_idx) {
				auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx);
				auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
				auto& p = mesh_edit.position_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].pos_idx];
				std::memcpy(pos_out, &p, sizeof(p)); },
			[](void* p_ctx, float normal_out[], const int f_idx, const int outer_b2v_idx) {
				auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx);
				auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
				auto& v_attr = mesh_edit.vertex_attr_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].attribute_idx];
				std::memcpy(normal_out, &v_attr.normal, sizeof(v_attr.normal)); },
			[](void* p_ctx, float uv_out[], const int f_idx, const int outer_b2v_idx) {
				auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx);
				auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
				auto& v_attr = mesh_edit.vertex_attr_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].attribute_idx];
				std::memcpy(uv_out, &v_attr.uv_set[0], sizeof(v_attr.uv_set[0])); },
			[](void* p_ctx, const float tangent_in[], const float fsign, const int f_idx, const int outer_b2v_idx) {
				auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx);
				auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
				auto& v_attr = mesh_edit.vertex_attr_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].attribute_idx];
		
				v_attr.tangent = { tangent_in[0], tangent_in[1], tangent_in[2], fsign }; },

			static_cast<void*>(&m), angular_threshold);
	}
}	 // namespace age::external::mikk