#include "hrp_common.asli"

struct gbuffer_opaque_out
{
	uint32_2 geo  sv_target_0;
	float2 motion sv_target_1;
};

gbuffer_opaque_out
main_ps(
	float4 pos		 sv_position,
	float3 normal	 semantics(normal),
	uint32 render_id semantics(render_id),
	uint32 prim_id	 sv_primitive_id)
{
	gbuffer_opaque_out res;
	// motion
	const opaque_meshlet_render_data render_data   = load_opaque_meshlet_render_data(render_id);
	const object_data				 obj_data_prev = load_object_prev_data(render_data.object_id);
	const object_data				 obj_data_curr = load_object_data(render_data.object_id);

	const float3 ndc	   = screen_to_ndc(pos.xy, pos.z, inv_backbuffer_size);
	const float3 world_pos = ndc_to_world(view_proj_inv, ndc);
	const float3 local_pos = rotate_inv(obj_data_curr.quaternion, world_pos - obj_data_curr.pos) / obj_data_curr.scale;

	const float3 world_pos_prev = rotate(obj_data_prev.quaternion, local_pos * obj_data_prev.scale) + obj_data_prev.pos;

	const float3 ndc_prev		= world_to_ndc(view_proj_prev, world_pos_prev);
	const float2 screen_uv_prev = ndc_xy_to_screen_uv(ndc_prev.xy);

	const float2 motion = pos.xy * inv_backbuffer_size - screen_uv_prev;

	res.geo.x  = (render_id & 0x01ffffff) | (prim_id << (32u - 7u));
	res.geo.y  = encode_oct_snorm16(normalize(normal));
	res.motion = motion;

	return res;
}
