#pragma once

#define ENV_LIGHT_BRDF_LUT_SAMPLE_COUNT	  1024
#define ENV_LIGHT_GGX_SAMPLE_COUNT		  2048
#define ENV_LIGHT_EIS_SAMPLE_COUNT		  2048
#define ENV_LIGHT_IRRADIANCE_SAMPLE_COUNT 1024
#define ENV_LIGHT_SAMPLE_COUNT			  (ENV_LIGHT_GGX_SAMPLE_COUNT + ENV_LIGHT_EIS_SAMPLE_COUNT)


#if !defined(AGE_SHADER)
	#include "age.hpp"

	#define reg(...)
	#define cbuffer struct
	#define row_major
	#define semantics(...)
	#define SHARED_TYPE shared_type::

namespace age::graphics::bake::shared_type
{
#else
	#define SHARED_TYPE
#endif

	cbuffer root_constants reg(b1)
	{
		uint32 brdf_lut_width;
		uint32 brdf_lut_height;
		uint32 brdf_lut_uav_id;

		uint32 env_light_input_texture_width;
		uint32 env_light_input_texture_height;
		uint32 env_light_input_texture_srv_id;
		uint32 env_light_radiance_texture_srv_id;
		uint32 env_light_radiance_texture_uav_id;
		uint32 env_light_irradiance_texture_uav_id;
		uint32 env_light_prefilter_texture_uav_id;	  // do not change member order
		uint32 env_light_prefilter_mip_count;		  // do not change member order
		uint32 env_light_prefilter_mip_max_count;

		uint32 env_light_radiance_size;
		uint32 env_light_irradiance_size;
		uint32 env_light_prefilter_size;

		uint32 down_sample_input_texture_srv_id;
		uint32 down_sample_output_texture_uav_id;	 // do not change member order
		uint32 down_sample_mip_count;				 // do not change member order
		uint32 down_sample_cube_output_size;		 // do not change member order
	};


#if !defined(AGE_SHADER)

	#undef ENV_LIGHT_BRDF_LUT_SAMPLE_COUNT
	#undef ENV_LIGHT_GGX_SAMPLE_COUNT
	#undef ENV_LIGHT_EIS_SAMPLE_COUNT
	#undef ENV_LIGHT_IRRADIANCE_SAMPLE_COUNT
	#undef ENV_LIGHT_SAMPLE_COUNT


}	 // age::graphics::bake::shared_type
#endif
