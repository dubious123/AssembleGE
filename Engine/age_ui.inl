#pragma once
#include "age.hpp"

namespace age::ui::detail
{
	// FNV-1a
	FORCE_INLINE constexpr uint64
	hash(const char* p_str) noexcept
	{
		uint64 res = g::fnv1a_offset_basis;
		while (*p_str != 0)
		{
			res ^= static_cast<uint64>(*p_str++);
			res *= g::fnv1a_prime;
		}
		return res;
	}

	FORCE_INLINE constexpr uint64
	hash(uint64 val) noexcept
	{
		uint64 res = g::fnv1a_offset_basis;
		for (int i = 0; i < 8; ++i)
		{
			res	 ^= val & 0xff;
			res	 *= g::fnv1a_prime;
			val >>= 8;
		}
		return res;
	}

	FORCE_INLINE constexpr uint64
	hash_combine(uint64 parent, uint64 child) noexcept
	{
		return (parent ^ child) * g::fnv1a_prime;
	}
}	 // namespace age::ui::detail

namespace age::ui::detail
{
	FORCE_INLINE root_data&
	get_current_root() noexcept
	{
		c_auto id = g::root_data_idx_stack.back();
		return g::root_data_vec_arr[(id >> 24u) & 0xff][id & 0x00ff'ffff];
	}

	FORCE_INLINE t_hash
	widget_begin(auto&& desc) noexcept
		requires std::is_same_v<BARE_OF(desc), widget_desc>
	{
		auto& root = get_current_root();

		c_auto id = new_id();

		auto z_offset		   = 0u;
		auto render_data_count = 0u;
		auto padding_sum	   = 0.f;
		auto atlas_id		   = 0u;

		{
			auto& size_data_parent = g::layout_size_data_stack[g::layout_size_data_current_idx];
			auto& pos_data_parent  = root.layout_pos_data_vec[size_data_parent.pos_data_idx];
			++pos_data_parent.child_count;

			z_offset = pos_data_parent.z_offset + desc.z_offset;

			if (desc.shape_kind == e::shape_kind::text)
			{
				if (AGE_IS_INVALID_IDX(desc.text.text_data_idx))
				{
					detail::gen_text_data(desc);
				}

				auto& text_data	  = g::text_data_vec[desc.text.text_data_idx];
				render_data_count = text_data.char_data_count;

				atlas_id = g::font_data_vec[desc.text.font_idx].second.atlas_id;
			}
			else
			{
				render_data_count = 1;
			}

			if (desc.draw is_false)
			{
				render_data_count = 0;
			}

			padding_sum = desc.layout == e::widget_layout::vertical or desc.layout == e::widget_layout::vertical_inv
							? desc.padding_left + desc.padding_right
							: desc.padding_top + desc.padding_bottom;
		}


		g::layout_size_data_stack.emplace_back(layout_size_data{
			.layout				= desc.layout,
			.width_mode			= desc.width_size_mode,
			.height_mode		= desc.height_size_mode,
			.child_subtree_size = 0,
			.parent_idx			= g::layout_size_data_current_idx,
			.pos_data_idx		= root.layout_pos_data_vec.size<uint32>(),
			.child_gap			= desc.child_gap,
			.width_min			= desc.width_min,
			.width_max			= desc.width_max,
			.width_final		= 0.f,
			.height_min			= desc.height_min,
			.height_max			= desc.height_max,
			.height_final		= 0.f,
		});

		root.layout_pos_data_vec.emplace_back(layout_pos_data{
			.id				   = id,
			.render_data_idx   = g::render_data_vec.size<uint32>(),
			.render_data_count = render_data_count,
			.layout			   = desc.layout,
			.align			   = desc.align,
			.z_offset		   = static_cast<uint16>(z_offset),
			.offset			   = desc.offset,
			.child_gap		   = desc.child_gap,
			.padding_left	   = desc.padding_left,
			.padding_right	   = desc.padding_right,
			.padding_top	   = desc.padding_top,
			.padding_bottom	   = desc.padding_bottom,
			.interact		   = desc.interact,
			.save_state		   = desc.save_state,
			.direct_draw	   = false,
			.text			   = { .idx = desc.text.text_data_idx, .atlas_id = atlas_id },
		});

		if (desc.draw)
		{
			g::render_data_vec.emplace_back(render_data{
				.pivot_uv		   = desc.pivot_uv,
				.rotation		   = desc.rotation,
				.border_thickness  = desc.border_thickness,
				.shape_kind		   = desc.shape_kind,
				.body_brush_kind   = desc.body_brush_kind,
				.border_brush_kind = desc.border_brush_kind,
				.shape_data		   = desc.shape_data,
				.body_brush_data   = desc.body_brush_data,
				.border_brush_data = desc.border_brush_data,
			});
		}

		// handle z_order
		{
			if (z_offset >= root.z_order_count_vec.size())
			{
				c_auto before_size = root.z_order_count_vec.size();
				root.z_order_count_vec.resize(z_offset + 1);
				std::ranges::fill(root.z_order_count_vec.begin() + before_size, root.z_order_count_vec.end(), 0u);
			}

			root.z_order_count_vec[z_offset] += render_data_count;
		}

		g::layout_size_data_current_idx = g::layout_size_data_stack.size<uint32>() - 1;

		return id;
	}

	t_hash
	widget_begin(auto&& mod) noexcept
		requires meta::is_not_same_v<BARE_OF(mod), widget_desc>
	{
		auto desc = widget_desc{};
		FWD(mod).apply(desc);
		return widget_begin(std::move(desc));
	}
}	 // namespace age::ui::detail

namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	begin(auto&& mod) noexcept
	{
		return widget_ctx{ ui::detail::widget_begin(FWD(mod)) };
	}
}	 // namespace age::ui::widget

namespace age::ui::font
{
	namespace detail
	{
		uint32
		find_idx(t_hash h) noexcept;
	}

	void
	load(const char* p_font_name, auto& renderer, asset::e::font_charset_flag flag, std::span<uint16> extra_unicode) noexcept
	{
		c_auto h   = ui::hash(p_font_name);
		c_auto idx = detail::find_idx(h);

		if (idx == age::get_invalid_id<uint32>())
		{
			auto h_font	  = asset::font::load(p_font_name, renderer, flag, extra_unicode);
			auto atlas_id = h_font.get_entry<asset::e::kind::font>().atlas_id;
			g::font_data_vec.emplace_back(std::pair{
				h, font_data{ .h_font = h_font, .atlas_id = atlas_id } });

			// auto h_font = asset::font::load(p_font_name, flag, extra_unicode);

			// c_auto& font_header = asset::font::get_asset_header(h_font);

			// c_auto atlas_id = renderer.upload_texture(font_header.get_atlas().data(), { .width = font_header.atlas_width, .height = font_header.atlas_height }, age::graphics::e::texture_format::rgba8_unorm);
		}
	}

	void
	unload(const char* p_font_name, auto& renderer) noexcept
	{
		c_auto h = ui::detail::hash(p_font_name);

		c_auto idx = detail::find_idx(h);

		AGE_ASSERT(idx != age::get_invalid_id<uint32>());

		auto font_data = g::font_data_vec[idx].second;
		asset::font::full_unload(font_data.h_font, renderer);

		// renderer.release_texture(g::font_data_vec[idx].second.atlas_id);
		// asset::unload(g::font_data_vec[idx].second.h_font);

		g::font_data_vec[idx] = g::font_data_vec.back();
		g::font_data_vec.pop_back();

		g::current_font_idx = std::min(g::current_font_idx, g::font_data_vec.size<uint32>() - 1);
	}
}	 // namespace age::ui::font

namespace age::ui
{
	void
	deinit(auto& renderer) noexcept
	{
		g::id_stack.clear();
		g::layout_size_data_stack.clear();
		g::render_data_vec.clear();
		g::root_data_idx_stack.clear();

		std::ranges::fill(g::root_data_vec_size_arr, 0u);

		for (auto& v : g::root_data_vec_arr)
		{
			v.clear();
		}

		g::text_data_vec.clear();
		g::word_data_vec.clear();
		g::char_data_vec.clear();
		g::char_pos_data_vec.clear();

		g::widget_state_map.clear();

		for (auto&& [hash, font_data] : g::font_data_vec)
		{
			asset::font::full_unload(font_data.h_font, renderer);
		}

		g::font_data_vec.clear();
	}
}	 // namespace age::ui
