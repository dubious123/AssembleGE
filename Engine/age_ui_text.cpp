#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui::detail
{
	void
	gen_text_data(widget_desc& desc) noexcept
	{
		c_auto* p_str		  = desc.text.p_str;
		c_auto	font_size	  = desc.text.font_size;
		c_auto	font_idx	  = desc.text.font_idx;
		c_auto	padding_sum_h = desc.padding_left + desc.padding_right;
		c_auto	padding_sum_v = desc.padding_top + desc.padding_bottom;
		c_auto	text_data_idx = g::text_data_vec.size<uint32>();

		auto text_data = ui::text_data{
			.char_data_offset = g::char_data_vec.size<uint32>(),
			.word_data_offset = g::word_data_vec.size<uint32>(),
			.line_height	  = ui::font::get_line_height(font_size, font_idx),
			.space_advance	  = ui::font::get_space_advance(font_size, font_idx)
		};

		auto word_width_min			  = 0.0f;
		auto max_width				  = 0.f;
		auto line_width				  = 0.f;
		auto word_count				  = 0u;
		auto char_count				  = 0u;
		auto word_char_count		  = 0u;
		auto word_leading_space_count = 0u;

		auto line_offset	= 0u;
		auto word_width		= 0.0f;
		auto word_byte_size = 0u;
		auto byte_offset	= 0u;

		for (auto c = *p_str;; c = *p_str)
		{
			if (c == ' ' or c == '\n' or c == '\0')
			{
				if (word_char_count > 0 or c == '\n' or c == '\0')
				{
					g::word_data_vec.emplace_back(word_data{
						.char_count			 = word_char_count,
						.width				 = word_width,
						.leading_space_count = word_leading_space_count,
						.line_offset		 = line_offset,
						.byte_offset		 = byte_offset,
						.byte_size			 = word_byte_size });

					c_auto leading_space  = word_leading_space_count * text_data.space_advance;
					word_width_min		  = std::max(word_width_min, word_width + leading_space);
					line_width			 += word_width + leading_space;

					word_width				  = 0.0f;
					word_char_count			  = 0;
					byte_offset				 += word_byte_size;
					word_byte_size			  = 0;
					word_leading_space_count  = 0;
					++word_count;
				}
			}
			if (c == ' ')
			{
				++word_leading_space_count;
				++p_str;
				++word_byte_size;
				continue;
			}
			if (c == '\n')
			{
				max_width				 = std::max(line_width + word_leading_space_count * text_data.space_advance, max_width);
				word_leading_space_count = 0u;
				line_width				 = 0.f;
				++line_offset;
				++p_str;
				++byte_offset;
				continue;
			}
			if (c == '\0')
			{
				max_width = std::max(line_width + word_leading_space_count * text_data.space_advance, max_width);
				break;
			}

			++char_count;
			++word_char_count;

			auto&& [byte_count, unicode]  = age::util::decode_utf8(p_str);
			p_str						 += byte_count;

			c_auto& glyph_data = ui::font::get_glyph_data(unicode, font_idx);

			word_width	   += glyph_data.advance * font_size;
			word_byte_size += byte_count;

			g::char_data_vec.emplace_back(ui::char_data{
				.advance	  = glyph_data.advance * font_size,
				.offset		  = glyph_data.offset * font_size,
				.size		  = glyph_data.size * font_size,
				.atlas_uv_min = glyph_data.atlas_uv_min,
				.atlas_uv_max = glyph_data.atlas_uv_max,
				.byte_size	  = byte_count,
			});
		}

		text_data.word_data_count = word_count;
		text_data.char_data_count = char_count;

		g::text_data_vec.emplace_back(std::move(text_data));

		c_auto height_min = (line_offset + 1u) * text_data.line_height;
		c_auto height_max = (line_offset + 1u + word_count) * text_data.line_height;

		desc.width_min	= word_width_min + padding_sum_h;
		desc.width_max	= max_width + padding_sum_h;
		desc.height_min = height_min + padding_sum_v;
		desc.height_max = height_max + padding_sum_v;

		desc.text.text_data_idx = text_data_idx;
	}
}	 // namespace age::ui::detail

namespace age::ui::detail
{
	cursor_data
	screen_offset_to_cursor(uint32 text_data_idx, float2 screen_offset, float width) noexcept
	{
		c_auto& text_data		   = g::text_data_vec[text_data_idx];
		c_auto	target_line_offset = static_cast<uint32>(std::max(screen_offset.y, 0.f) / text_data.line_height);

		auto res = cursor_data{};

		auto line_offset = 0u;
		auto wrap_count	 = 0u;
		auto cursor_x	 = 0.f;
		auto word_found	 = false;

		for (auto char_offset = text_data.char_data_offset;
			 c_auto& word_data : std::span(g::word_data_vec.data() + text_data.word_data_offset, text_data.word_data_count))
		{
			c_auto leading_space = word_data.leading_space_count * text_data.space_advance;

			if (word_data.line_offset + wrap_count > line_offset)
			{
				line_offset = word_data.line_offset + wrap_count;

				if (target_line_offset < line_offset)
				{
					break;
				}

				cursor_x			 = 0.f;
				res.line_byte_offset = word_data.byte_offset;
				res.line_byte_size	 = 0;
			}
			else if (cursor_x > 0.f and cursor_x + leading_space + word_data.width > width + math::g::epsilon_1e4)
			{
				++line_offset;
				++wrap_count;

				if (target_line_offset < line_offset)
				{
					break;
				}

				cursor_x			 = 0.f;
				res.line_byte_offset = word_data.byte_offset;
				res.line_byte_size	 = 0;
			}

			if (word_found is_false and target_line_offset == line_offset)
			{
				c_auto leading_space_byte_size = word_data.leading_space_count;
				if (screen_offset.x < cursor_x + leading_space)
				{
					// clicked leading space
					res.word_byte_offset = word_data.byte_offset;
					res.word_byte_size	 = word_data.leading_space_count;

					res.word_min_x = cursor_x;
					res.word_max_x = cursor_x + leading_space;

					c_auto space_count = static_cast<uint32>((screen_offset.x - cursor_x + text_data.space_advance * 0.5f) / text_data.space_advance);

					res.byte_offset = res.word_byte_offset + space_count;
					res.offset.x	= cursor_x + space_count * text_data.space_advance;

					word_found = true;
				}
				else if (screen_offset.x < cursor_x + leading_space + word_data.width)
				{
					// clicked word
					res.word_byte_offset = word_data.byte_offset + word_data.leading_space_count;
					res.word_byte_size	 = word_data.byte_size - word_data.leading_space_count;

					res.word_min_x = cursor_x + leading_space;
					res.word_max_x = res.word_min_x + word_data.width;

					res.byte_offset = res.word_byte_offset;
					res.offset.x	= cursor_x + leading_space;

					for (c_auto& char_data : std::span(g::char_data_vec.data() + char_offset, word_data.char_count))
					{
						if (screen_offset.x < res.offset.x + char_data.advance * 0.5f)
						{
							break;
						}

						res.offset.x	+= char_data.advance;
						res.byte_offset += char_data.byte_size;
					}

					word_found = true;
				}
			}

			cursor_x	+= leading_space + word_data.width;
			char_offset += word_data.char_count;

			res.line_byte_size += word_data.byte_size;
			res.line_width	   += leading_space + word_data.width;
		}

		res.offset.y = target_line_offset * text_data.line_height;

		if (word_found is_false)
		{
			res.byte_offset = res.line_byte_offset + res.line_byte_size;
			res.offset.x	= cursor_x;
		}

		return res;
	}

	cursor_data
	byte_offset_to_cursor(uint32 text_data_idx, uint32 byte_offset, float width) noexcept
	{
		c_auto& text_data = g::text_data_vec[text_data_idx];

		auto res		= cursor_data{};
		res.byte_offset = byte_offset;

		auto cursor_x	 = 0.f;
		auto line_offset = 0u;
		auto wrap_count	 = 0u;

		auto word_found = false;

		auto char_offset = text_data.char_data_offset;
		auto word_idx	 = 0u;
		for (auto i : views::loop(text_data.word_data_count))
		{
			word_idx			 = i;
			c_auto word_data	 = g::word_data_vec[text_data.word_data_offset + word_idx];
			c_auto leading_space = word_data.leading_space_count * text_data.space_advance;

			auto new_line = word_idx == 0;

			if (word_data.line_offset + wrap_count > line_offset)
			{
				// line_offset = word_data.line_offset + wrap_count;
				new_line = true;
			}
			else if (cursor_x > 0.f and cursor_x + leading_space + word_data.width > width + math::g::epsilon_1e4)
			{
				++wrap_count;
				//++line_offset;
				new_line = true;
			}

			if (new_line and byte_offset < word_data.byte_offset)
			{
				break;
			}

			if (new_line)
			{
				cursor_x			 = 0.f;
				res.line_byte_offset = word_data.byte_offset;
				res.line_byte_size	 = 0;
				line_offset			 = word_data.line_offset + wrap_count;
			}

			if (word_found is_false and byte_offset >= word_data.byte_offset)
			{
				c_auto leading_space_byte_size = word_data.leading_space_count;
				if (byte_offset < word_data.byte_offset + word_data.leading_space_count)
				{
					// handle leading space
					res.word_byte_offset = word_data.byte_offset;
					res.word_byte_size	 = word_data.leading_space_count;

					res.word_min_x = cursor_x;
					res.word_max_x = cursor_x + leading_space;

					c_auto space_count = byte_offset - word_data.byte_offset;

					res.offset.x = cursor_x + space_count * text_data.space_advance;

					word_found = true;
				}
				else if (byte_offset < word_data.byte_offset + word_data.char_count + word_data.leading_space_count)
				{
					// handle word
					res.word_byte_offset = word_data.byte_offset + word_data.leading_space_count;
					res.word_byte_size	 = word_data.byte_size - word_data.leading_space_count;

					res.word_min_x = cursor_x + leading_space;
					res.word_max_x = res.word_min_x + word_data.width;

					res.offset.x = cursor_x + leading_space;

					c_auto char_count = byte_offset - (word_data.byte_offset + word_data.leading_space_count);

					for (auto byte_idx = word_data.byte_offset;
						 c_auto& char_data : std::span(g::char_data_vec.data() + char_offset, char_count))
					{
						res.offset.x += char_data.advance;
						byte_idx	 += char_data.byte_size;
					}

					word_found = true;
				}
			}

			cursor_x	+= leading_space + word_data.width;
			char_offset += word_data.char_count;

			res.line_byte_size += word_data.byte_size;
			res.line_width	   += leading_space + word_data.width;
		}

		if (word_found is_false)
		{
			res.offset.x = cursor_x;

			AGE_ASSERT(text_data.word_data_count > 0);
			AGE_ASSERT(text_data.word_data_offset + word_idx > 1);

			c_auto& word_data = g::word_data_vec[text_data.word_data_offset + word_idx - 1];

			if (word_data.char_count > 0)
			{
				res.word_byte_offset = word_data.byte_offset + word_data.leading_space_count;
				res.word_byte_size	 = word_data.byte_size - word_data.leading_space_count;
			}
			else
			{
				res.word_byte_offset = word_data.byte_offset;
				res.word_byte_size	 = word_data.leading_space_count;
			}
		}

		// AGE_ASSERT(word_found);


		res.offset.y = line_offset * text_data.line_height;

		return res;
	}
}	 // namespace age::ui::detail

namespace age::ui::detail
{
	FORCE_INLINE uint32
	skip_word_left(const char* p_buf, uint32 pos) noexcept
	{
		if (pos == 0) { return 0; }

		--pos;
		if (p_buf[pos] == ' ')
		{
			while (pos > 0 and p_buf[pos - 1] == ' ')
			{
				--pos;
			}
		}
		else if (p_buf[pos] == '\n')
		{
		}
		else
		{
			while (pos > 0 and p_buf[pos - 1] != ' ' and p_buf[pos - 1] != '\n')
			{
				--pos;
			}
		}
		return pos;
	}

	FORCE_INLINE uint32
	skip_word_right(const char* p_buf, uint32 pos, uint32 len) noexcept
	{
		if (pos >= len) { return len; }

		++pos;
		if (p_buf[pos - 1] == ' ')
		{
			while (pos < len and p_buf[pos] == ' ')
			{
				++pos;
			}
		}
		else if (p_buf[pos - 1] == '\n')
		{
		}
		else
		{
			while (pos < len and p_buf[pos] != ' ' and p_buf[pos] != '\n')
			{
				++pos;
			}
		}
		return pos;
	}

	void
	update_text_buf(char* p_buf, uint32 buf_byte_size, cursor_data& cursor) noexcept
	{
		using enum input::e::key_kind;
		auto str_len = static_cast<uint32>(std::strlen(p_buf));

		auto& cursor_byte_idx = cursor.byte_offset;
		auto& anchor_byte_idx = cursor.anchor_byte_offset;

		// cursor movement (will be applied on the next frame)
		c_auto prev_cursor = cursor_byte_idx;

		if (g::p_input_ctx->is_pressed_or_repeat(key_home))
		{
			if (g::p_input_ctx->is_ctrl_down())
			{
				cursor_byte_idx = 0;
			}
			else
			{
				cursor_byte_idx = cursor.line_byte_offset;
			}
		}

		if (g::p_input_ctx->is_pressed_or_repeat(key_end))
		{
			if (g::p_input_ctx->is_ctrl_down())
			{
				cursor_byte_idx = static_cast<uint32>(std::strlen(p_buf));
			}
			else
			{
				cursor_byte_idx = cursor.line_byte_offset + cursor.line_byte_size;
			}
		}

		if (g::p_input_ctx->is_pressed_or_repeat(key_left) and cursor_byte_idx > 0)
		{
			if (g::p_input_ctx->is_ctrl_down())
			{
				cursor_byte_idx = skip_word_left(p_buf, cursor_byte_idx);
			}
			else
			{
				--cursor_byte_idx;
				while (cursor_byte_idx > 0 and (p_buf[cursor_byte_idx] & 0xC0) == 0x80)
				{
					--cursor_byte_idx;
				}
			}
		}

		if (g::p_input_ctx->is_pressed_or_repeat(key_right) and cursor_byte_idx < str_len)
		{
			if (g::p_input_ctx->is_ctrl_down())
			{
				cursor_byte_idx = skip_word_right(p_buf, cursor_byte_idx, str_len);
			}
			else
			{
				++cursor_byte_idx;
				while (cursor_byte_idx < str_len and (p_buf[cursor_byte_idx] & 0xC0) == 0x80)
				{
					++cursor_byte_idx;
				}
			}
		}

		if (cursor_byte_idx != prev_cursor and not g::p_input_ctx->is_shift_down())
		{
			anchor_byte_idx = cursor_byte_idx;
		}

		// edit
		auto selection_consumed = false;

		if (c_auto has_selection = anchor_byte_idx != cursor_byte_idx)
		{
			if (g::p_input_ctx->is_pressed_or_repeat(key_backspace)
				or g::p_input_ctx->is_pressed_or_repeat(key_delete)
				or g::p_input_ctx->is_pressed_or_repeat(key_enter)
				or g::utf8_buf_len)
			{
				// delete selection
				c_auto sel_min = std::min(cursor_byte_idx, anchor_byte_idx);
				c_auto sel_max = std::max(cursor_byte_idx, anchor_byte_idx);
				std::memmove(p_buf + sel_min, p_buf + sel_max, str_len - sel_max + 1);
				str_len			-= (sel_max - sel_min);
				cursor_byte_idx	 = sel_min;
				anchor_byte_idx	 = sel_min;

				selection_consumed = true;
			}
		}

		auto anchor_fixed = anchor_byte_idx != cursor_byte_idx;

		if (not selection_consumed and g::p_input_ctx->is_pressed_or_repeat(key_backspace) and cursor_byte_idx > 0)
		{
			c_auto prev_pos = cursor_byte_idx;

			if (g::p_input_ctx->is_ctrl_down())
			{
				cursor_byte_idx = detail::skip_word_left(p_buf, cursor_byte_idx);
			}
			else
			{
				--cursor_byte_idx;
				while (cursor_byte_idx > 0 and (p_buf[cursor_byte_idx] & 0xC0) == 0x80)
				{
					--cursor_byte_idx;
				}
			}

			c_auto remove_count = prev_pos - cursor_byte_idx;
			std::memmove(p_buf + cursor_byte_idx, p_buf + prev_pos, str_len - prev_pos + 1);
			str_len -= remove_count;
		}

		if (not selection_consumed and g::p_input_ctx->is_pressed_or_repeat(key_delete) and cursor_byte_idx < str_len)
		{
			auto end_pos = cursor_byte_idx;

			if (g::p_input_ctx->is_ctrl_down())
			{
				end_pos = detail::skip_word_right(p_buf, cursor_byte_idx, str_len);
			}
			else
			{
				++end_pos;
				while (end_pos < str_len and (p_buf[end_pos] & 0xC0) == 0x80)
				{
					++end_pos;
				}
			}

			c_auto remove_count = end_pos - cursor_byte_idx;
			std::memmove(p_buf + cursor_byte_idx, p_buf + end_pos, str_len - end_pos + 1);
			str_len -= remove_count;
		}

		if (g::p_input_ctx->is_pressed_or_repeat(key_enter))
		{
			std::memmove(p_buf + cursor_byte_idx + 1, p_buf + cursor_byte_idx, str_len - cursor_byte_idx + 1);
			p_buf[cursor_byte_idx] = '\n';
			++cursor_byte_idx;
			++str_len;
		}

		if (g::utf8_buf_len > 0 and str_len + 1 + g::utf8_buf_len < buf_byte_size)
		{
			std::memmove(p_buf + cursor_byte_idx + g::utf8_buf_len, p_buf + cursor_byte_idx, str_len - cursor_byte_idx + 1);
			std::memcpy(p_buf + cursor_byte_idx, g::utf8_buf, g::utf8_buf_len);

			cursor_byte_idx += g::utf8_buf_len;

			for (auto* ptr = g::utf8_buf; ptr < g::utf8_buf + g::utf8_buf_len;)
			{
				auto&& [byte_count, _]	= age::util::decode_utf8(ptr);
				ptr					   += byte_count;
			}
		}

		if (anchor_fixed is_false)
		{
			anchor_byte_idx = cursor_byte_idx;
		}
	}

	float
	calc_line_width(const char*& p_buf, float font_size, uint32 font_idx) noexcept
	{
		auto width = 0.f;

		for (auto c = *p_buf; c != '\n' and c != '\0'; c = *p_buf)
		{
			c_auto[byte_count, unicode]	 = age::util::decode_utf8(p_buf);
			width						+= ui::font::get_advance(unicode, font_size, font_idx);
			p_buf						+= byte_count;
		}

		return width;
	}
}	 // namespace age::ui::detail