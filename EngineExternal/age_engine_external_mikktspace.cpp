extern "C"
{
#include "mikktspace/mikktspace.h"
#include "mikktspace/mikktspace.c"
}

namespace age::external::mikk::detail
{
	bool
	gen_tangent_space(
		int	 (* const fp_get_face_count)(void* p_data),
		int	 (* const fp_get_vertex_count_of_face)(void* p_data, const int face_idx),
		void (* const fp_get_position)(void* p_data, float pos_out[], const int face_idx, const int corner_idx),
		void (* const fp_get_normal)(void* p_data, float normal_out[], const int face_idx, const int corner_idx),
		void (* const fp_get_uv)(void* p_data, float uv_out[], const int face_idx, const int corner_idx),
		void (* const fp_set_tangent)(void* p_data, const float tangent_in[], const float tangent_sign, const int face_idx, const int corner_idx),

		void* const p_data,
		const float angular_threshold) noexcept
	{
		struct t_user_ctx
		{
			int			(* const fp_get_face_count)(void* p_data);
			int			(* const fp_get_vertex_count_of_face)(void* p_data, const int face_idx);
			void		(* const fp_get_position)(void* p_data, float pos_out[], const int face_idx, const int corner_idx);
			void		(* const fp_get_normal)(void* p_data, float normal_out[], const int face_idx, const int corner_idx);
			void		(* const fp_get_uv)(void* p_data, float uv_out[], const int face_idx, const int corner_idx);
			void		(* const fp_set_tangent)(void* p_data, const float tangent_in[], const float tangent_sign, const int face_idx, const int corner_idx);
			void* const p_data;
		} user_ctx{
			fp_get_face_count,
			fp_get_vertex_count_of_face,
			fp_get_position,
			fp_get_normal,
			fp_get_uv,
			fp_set_tangent,
			p_data
		};

		auto mikkt_space_interface = SMikkTSpaceInterface{
			.m_getNumFaces =
				+[](const SMikkTSpaceContext* p_ctx) {
					auto* p_user_ctx = static_cast<t_user_ctx*>(p_ctx->m_pUserData);
					return p_user_ctx->fp_get_face_count(p_user_ctx->p_data);
				},
			.m_getNumVerticesOfFace =
				+[](const SMikkTSpaceContext* p_ctx, const int face_idx) {
					auto* p_user_ctx = static_cast<t_user_ctx*>(p_ctx->m_pUserData);
					return p_user_ctx->fp_get_vertex_count_of_face(p_user_ctx->p_data, face_idx);
				},
			.m_getPosition =
				+[](const SMikkTSpaceContext* p_ctx, float pos_out[], const int face_idx, const int corner_idx) {
					auto* p_user_ctx = static_cast<t_user_ctx*>(p_ctx->m_pUserData);
					return p_user_ctx->fp_get_position(p_user_ctx->p_data, pos_out, face_idx, corner_idx);
				},
			.m_getNormal =
				+[](const SMikkTSpaceContext* p_ctx, float normal_out[], const int face_idx, const int corner_idx) {
					auto* p_user_ctx = static_cast<t_user_ctx*>(p_ctx->m_pUserData);
					return p_user_ctx->fp_get_normal(p_user_ctx->p_data, normal_out, face_idx, corner_idx);
				},
			.m_getTexCoord =
				+[](const SMikkTSpaceContext* p_ctx, float uv_out[], const int face_idx, const int corner_idx) {
					auto* p_user_ctx = static_cast<t_user_ctx*>(p_ctx->m_pUserData);
					return p_user_ctx->fp_get_uv(p_user_ctx->p_data, uv_out, face_idx, corner_idx);
				},
			.m_setTSpaceBasic =
				+[](const SMikkTSpaceContext* p_ctx, const float tangent_in[], const float tangent_sign, const int face_idx, const int corner_idx) {
					auto* p_user_ctx = static_cast<t_user_ctx*>(p_ctx->m_pUserData);
					return p_user_ctx->fp_set_tangent(p_user_ctx->p_data, tangent_in, tangent_sign, face_idx, corner_idx);
				},
		};

		const auto ctx = SMikkTSpaceContext{
			.m_pInterface = &mikkt_space_interface,
			.m_pUserData  = static_cast<void*>(&user_ctx)
		};

		return genTangSpace(&ctx, angular_threshold);
	}
}	 // namespace age::external::mikk::detail