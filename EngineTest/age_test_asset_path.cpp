#include "age_test_pch.hpp"
#include "age_test.hpp"

namespace age_test::asset::path
{
	using namespace age;

	// ============================================================
	// normalize_asset_path
	// ============================================================
	namespace
	{
		constexpr auto test_kind = age::asset::e::kind::mesh_baked;

		std::string_view
		test_tag() noexcept
		{
			return age::asset::visit(test_kind, []<age::asset::e::kind k> { return std::string_view{ age::asset::get_asset_tag<k>() }; });
		}

		std::string_view
		test_ext() noexcept
		{
			return std::string_view{ config::asset_extension };
		}

		std::string
		test_suffix() noexcept
		{
			return std::string{ test_tag() } + std::string{ test_ext() };
		}

		// the longest body that still fits without falling back
		size_t
		test_max_body_len() noexcept
		{
			return config::max_asset_path_len - 1 - test_tag().size() - test_ext().size();
		}

		age::array<char, config::max_asset_path_len>
		make_path(std::string_view sv) noexcept
		{
			auto arr = age::make_filled_array<char, config::max_asset_path_len>('\0');
			AGE_ASSERT(sv.size() < arr.size());
			std::ranges::copy(sv, arr.begin());
			return arr;
		}

		// asserts normalize(input) == expected_body + tag + extension, and that normalizing again is a no-op
		void
		check_norm(std::string_view input, std::string_view expected_body) noexcept
		{
			auto arr = make_path(input);
			age::asset::normalize_asset_path(test_kind, arr);

			c_auto once = std::string{ std::string_view{ arr.data() } };
			AGE_ASSERT(once == std::string{ expected_body } + test_suffix(), "once : {}, expected_body + suffix : {}, suffix : {}", once, std::string{ expected_body } + test_suffix(), test_suffix());

			age::asset::normalize_asset_path(test_kind, arr);	 // idempotency
			AGE_ASSERT(std::string_view{ arr.data() } == once);
		}
	}	 // namespace

	// ============================================================
	// 1. separator
	// ============================================================
	void
	test_normalize_separator()
	{
		check_norm("a\\b\\c", "a/b/c");
		check_norm("a\\b/c", "a/b/c");
	}

	// ============================================================
	// 2. lower case
	// ============================================================
	void
	test_normalize_lower_case()
	{
		check_norm("ROCK/WALL", "rock/wall");
		check_norm("Rock", "rock");
	}

	// ============================================================
	// 3. invalid char -> '_'
	// ============================================================
	void
	test_normalize_invalid_char()
	{
		check_norm("a?b", "a_b");
		check_norm("a<b>c", "a_b_c");
		check_norm("a b", "a_b");	 // space is not in the allowed set
	}

	// ============================================================
	// 4. allowed char survives
	// ============================================================
	void
	test_normalize_allowed_char()
	{
		check_norm("a-b_c123", "a-b_c123");
		check_norm("mesh/rock_01", "mesh/rock_01");
	}

	// ============================================================
	// 5. duplicate slash
	// ============================================================
	void
	test_normalize_duplicate_slash()
	{
		check_norm("a//b", "a/b");
		check_norm("a////b", "a/b");
	}

	// ============================================================
	// 6. leading / trailing slash
	// ============================================================
	void
	test_normalize_edge_slash()
	{
		check_norm("/a/b", "a/b");
		check_norm("a/b/", "a/b");
		check_norm("/a/b/", "a/b");
	}

	// ============================================================
	// 7. "." and ".." segments
	// ============================================================
	void
	test_normalize_dot_segment()
	{
		check_norm("a/./b", "a/b");
		check_norm("./a", "a");
		check_norm("a/../b", "a/b");	// silently dropped here - validate rejects ".."
	}

	// ============================================================
	// 8. trailing dots of a segment
	// ============================================================
	void
	test_normalize_segment_trailing_dot()
	{
		check_norm("a./b", "a/b");
		check_norm("a.../b", "a/b");
	}

	// ============================================================
	// 9. dot inside a directory segment -> '_'
	// ============================================================
	void
	test_normalize_dot_in_directory()
	{
		check_norm("dir.v2/name", "dir_v2/name");
		check_norm("a.b/c.d", "a_b/c");	   // the last dot is the extension and is stripped
	}

	// ============================================================
	// 10. the extension is always the kind's tag
	// ============================================================
	void
	test_normalize_extension()
	{
		check_norm("rock.obj", "rock");
		check_norm("rock", "rock");
		check_norm("a.old.msh", "a");
	}

	// ============================================================
	// 11. filename that is only an extension
	// ============================================================
	void
	test_normalize_extension_only()
	{
		check_norm("a/.msh", "a");
		check_norm(".msh", "invalid_asset_name");
	}

	// ============================================================
	// 12. reserved words
	// ============================================================
	void
	test_normalize_reserved_word()
	{
		check_norm("con", "___");
		check_norm("con.msh", "___");
		check_norm("nul", "___");
		check_norm("com1", "____");
		check_norm("lpt9", "____");
		check_norm("con/a", "___/a");
	}

	// ============================================================
	// 13. near misses are not reserved
	// ============================================================
	void
	test_normalize_reserved_word_near_miss()
	{
		check_norm("console", "console");
		check_norm("com10", "com10");
		check_norm("acon", "acon");
	}

	// ============================================================
	// 14. fallback - nothing survives
	// ============================================================
	void
	test_normalize_fallback()
	{
		check_norm("", "invalid_asset_name");
		check_norm("///", "invalid_asset_name");
		check_norm("...", "invalid_asset_name");
		check_norm("/./../", "invalid_asset_name");
	}

	// ============================================================
	// 15. length budget - exactly at the limit must NOT fall back
	// ============================================================
	void
	test_normalize_length_boundary()
	{
		c_auto max_body = test_max_body_len();

		check_norm(std::string(max_body, 'a'), std::string(max_body, 'a'));	   // fits
		check_norm(std::string(max_body + 1, 'a'), "invalid_asset_name");	   // one over
	}

	// ============================================================
	// 16. no null terminator in the buffer
	// ============================================================
	void
	test_normalize_no_null_terminator()
	{
		auto arr = age::make_filled_array<char, config::max_asset_path_len>('a');

		age::asset::normalize_asset_path(test_kind, arr);

		c_auto res = std::string_view{ arr.data() };
		AGE_ASSERT(res == std::string{ "invalid_asset_name" } + test_suffix());
		AGE_ASSERT(res.size() < arr.size());
	}

	// ============================================================
	// 17. non-ascii and control chars
	// ============================================================
	void
	test_normalize_non_ascii()
	{
		check_norm("a\tb", "a_b");
		check_norm("a\x01"
				   "\x02"
				   "b",
				   "a__b");
		check_norm("\xEA\xB0\x80", "___");	  // utf-8 bytes - one '_' per byte
		check_norm("a\xFF"
				   "b",
				   "a_b");					  // negative char (signed char path)
	}

	// ============================================================
	// 18. every kind - the budget depends on the tag length
	// ============================================================
	void
	test_normalize_all_kinds()
	{
		age::asset::for_each_kind([]<age::asset::e::kind k> noexcept {
			auto arr = make_path("Dir/Name.obj");
			age::asset::normalize_asset_path(k, arr);

			c_auto res	  = std::string_view{ arr.data() };
			c_auto expect = std::string{ "dir/name" }
						  + std::string{ std::string_view{ age::asset::get_asset_tag<k>() } }
						  + std::string{ std::string_view{ config::asset_extension } };

			AGE_ASSERT(res == expect);

			auto again = arr;
			age::asset::normalize_asset_path(k, again);
			AGE_ASSERT(std::string_view{ again.data() } == res);
		});
	}

	// ============================================================
	// 19. output invariants hold for every input
	// ============================================================
	void
	test_normalize_invariants()
	{
		constexpr std::string_view inputs[]{
			"", "/", "a", "A/B", "a//b", "a/./b", "a/../b", "con.msh", "dir.v2/x",
			"???", "  ", "a . /b", "/a/b/", "....", "com1/con/prn"
		};

		for (c_auto in : inputs)
		{
			auto arr = make_path(in);
			age::asset::normalize_asset_path(test_kind, arr);

			c_auto res = std::string_view{ arr.data() };

			AGE_ASSERT(res.empty() is_false);
			AGE_ASSERT(res.ends_with(test_suffix()));
			AGE_ASSERT(res.size() < config::max_asset_path_len);
			AGE_ASSERT(res.find('\\') == std::string_view::npos);
			AGE_ASSERT(res.find("//") == std::string_view::npos);
			AGE_ASSERT(res.starts_with('/') is_false);
			AGE_ASSERT(std::ranges::none_of(res, [](char c) { return 'A' <= c and c <= 'Z'; }));

			auto again = arr;
			age::asset::normalize_asset_path(test_kind, again);
			AGE_ASSERT(std::string_view{ again.data() } == res);
		}
	}

	// ============================================================
	// 20. com0 / lpt0 - reserved under the stricter policy
	// ============================================================
	void
	test_normalize_reserved_zero()
	{
		check_norm("com0", "____");
		check_norm("lpt0", "____");
	}

	// ============================================================
	// 21. a path carrying another kind's suffix
	// ============================================================
	void
	test_normalize_foreign_suffix()
	{
		check_norm("rock.texture.age_asset", "rock");		// both groups stripped
		check_norm("rock.mesh_baked.age_asset", "rock");	// our own suffix
		check_norm("a/b.model.age_asset", "a/b");
	}

	// ============================================================
	// 22. reserved detection runs AFTER dots become '_'
	// ============================================================
	void
	test_normalize_reserved_with_dot()
	{
		check_norm("con.x/a", "con_x/a");	 // "con.x" is not a device name
		check_norm("CON/a", "___/a");		 // upper case is lowered first
		check_norm("Con.Msh", "___");
	}

	// ============================================================
	// 23. backslash collapses together with duplicate slashes
	// ============================================================
	void
	test_normalize_separator_collapse()
	{
		check_norm("a\\\\b", "a/b");
		check_norm("a\\/b", "a/b");
		check_norm("\\a\\b\\", "a/b");
	}

	// ============================================================
	// 24. leftover dots after the strip loop
	// ============================================================
	void
	test_normalize_trailing_dots()
	{
		check_norm("name...", "name");	  // strip takes two, trim takes the rest
		check_norm("name....", "name");
		check_norm("a/name..", "a/name");
	}

	// ============================================================
	// 25. deep nesting and an all-separator buffer
	// ============================================================
	void
	test_normalize_extreme_shape()
	{
		check_norm("a/b/c/d/e/f/g/h", "a/b/c/d/e/f/g/h");
		check_norm(std::string(config::max_asset_path_len - 2, '/'), "invalid_asset_name");
		check_norm(".mesh_baked.age_asset", "invalid_asset_name");	  // suffix with no name
	}

	void
	run_test()
	{
		test_normalize_separator();
		test_normalize_lower_case();
		test_normalize_invalid_char();
		test_normalize_allowed_char();
		test_normalize_duplicate_slash();
		test_normalize_edge_slash();
		test_normalize_dot_segment();
		test_normalize_segment_trailing_dot();
		test_normalize_dot_in_directory();
		test_normalize_extension();
		test_normalize_extension_only();
		test_normalize_reserved_word();
		test_normalize_reserved_word_near_miss();
		test_normalize_fallback();
		test_normalize_length_boundary();
		test_normalize_no_null_terminator();
		test_normalize_non_ascii();
		test_normalize_all_kinds();
		test_normalize_invariants();
		test_normalize_reserved_zero();
		test_normalize_foreign_suffix();
		test_normalize_reserved_with_dot();
		test_normalize_separator_collapse();
		test_normalize_trailing_dots();
		test_normalize_extreme_shape();
	}
}	 // namespace age_test::asset::path