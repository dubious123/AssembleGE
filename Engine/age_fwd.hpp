#pragma once

namespace age::data_structure
{

}

namespace age::graphics
{
	AGE_DEFINE_ENUM(color_space, uint8, srgb, hdr);
}	 // namespace age::graphics

namespace age::graphics::e
{
	AGE_DEFINE_ENUM(camera_kind, uint8, perspective, orthographic);

	AGE_DEFINE_ENUM_WITH_VALUE(light_kind, uint16,
							   (directional, 0),
							   (point, 1),
							   (spot, 2),
							   (area, 3),
							   (volumn, 4));
}	 // namespace age::graphics::e

namespace age::asset
{
	struct mesh_editable;


}	 // namespace age::asset
