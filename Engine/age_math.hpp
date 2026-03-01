#pragma once

namespace age
{
	inline namespace math
	{
		template <typename t>
		requires(not std::is_same_v<t, bool> and std::is_arithmetic_v<t>)
		struct extent_2d
		{
			t width;
			t height;

			constexpr auto
			operator<=>(const extent_2d<t>&) const noexcept = default;
		};
	}	 // namespace math
}	 // namespace age

using uint64 = uint64_t;
using uint32 = uint32_t;
using uint16 = uint16_t;
using uint8	 = uint8_t;

using int64 = int64_t;
using int32 = int32_t;
using int16 = int16_t;
using int8	= int8_t;

using float32  = float;
using double64 = double;
using half	   = uint16;

inline constexpr uint64 invalid_id_uint64 = 0xffff'ffff'ffff'ffffui64;
inline constexpr uint32 invalid_id_uint32 = 0xffff'ffffui32;
inline constexpr uint16 invalid_id_uint16 = 0xffffui16;
inline constexpr uint8	invalid_id_uint8  = 0xffui8;

// todo, move to better header
namespace age
{
	template <typename t>
	consteval t
	get_invalid_id() noexcept
	{
		if constexpr (std::is_same_v<t, uint64>)
		{
			return invalid_id_uint64;
		}
		else if constexpr (std::is_same_v<t, uint32>)
		{
			return invalid_id_uint32;
		}
		else if constexpr (std::is_same_v<t, uint16>)
		{
			return invalid_id_uint16;
		}
		else if constexpr (std::is_same_v<t, uint8>)
		{
			return invalid_id_uint8;
		}
		else
		{
			static_assert(false, "invalid id type");
		}
	}
}	 // namespace age

static_assert(sizeof(float32) == 4);
static_assert(sizeof(double64) == 8);

template <typename t>
requires(std::is_arithmetic_v<t>)
struct vec2
{
	using t_value = t;
	using t_this  = vec2<t>;

	t x;
	t y;

	FORCE_INLINE constexpr vec2() noexcept = default;

	FORCE_INLINE constexpr vec2(auto&& other) noexcept
		requires(requires { other.x; other.y; }
				 and std::convertible_to<decltype(other.x), t>
				 and std::convertible_to<decltype(other.y), t>)
		: x{ static_cast<t>(FWD(other).x) }, y{ static_cast<t>(FWD(other).y) }
	{
	}

	FORCE_INLINE constexpr vec2(auto&& other) noexcept
		requires(std::convertible_to<decltype(other), t>
				 and not requires { other.x; other.y; })
		: x{ static_cast<t>(FWD(other)) }, y{ static_cast<t>(FWD(other)) }
	{
	}

	FORCE_INLINE constexpr vec2(auto&& x_other, auto&& y_other) noexcept
		requires(std::convertible_to<decltype(x_other), t>
				 and std::convertible_to<decltype(y_other), t>)
		: x{ static_cast<t>(FWD(x_other)) }, y{ static_cast<t>(FWD(y_other)) }
	{
	}

	template <typename u>
	FORCE_INLINE constexpr vec2(const u (&other)[2]) noexcept
		requires(std::convertible_to<u, t>)
		: x{ static_cast<t>(other[0]) }, y{ static_cast<t>(other[1]) }
	{
	}

	FORCE_INLINE constexpr t*
	data() noexcept
	{
		return &x;
	}

	FORCE_INLINE constexpr const t*
	data() const noexcept
	{
		return &x;
	}

	FORCE_INLINE constexpr t&
	operator[](std::size_t c) noexcept
	{
		AGE_ASSERT(c < 2);
		return (&x)[c];
	}

	FORCE_INLINE constexpr const t&
	operator[](std::size_t c) const noexcept
	{
		AGE_ASSERT(c < 2);
		return (&x)[c];
	}

	FORCE_INLINE constexpr bool
	operator==(const t_this&) const noexcept = default;
	FORCE_INLINE constexpr auto
	operator<=>(const t_this&) const noexcept = default;

	FORCE_INLINE constexpr t_this
	operator+(const t_this& other_r) const noexcept
	{
		return { x + other_r.x, y + other_r.y };
	}

	FORCE_INLINE constexpr t_this
	operator-(const t_this& other_r) const noexcept
	{
		return { x - other_r.x, y - other_r.y };
	}

	FORCE_INLINE constexpr t_this&
	operator+=(const t_this& other_r) noexcept
	{
		x += other_r.x;
		y += other_r.y;
		return *this;
	}

	FORCE_INLINE constexpr t_this&
	operator-=(const t_this& other_r) noexcept
	{
		x -= other_r.x;
		y -= other_r.y;
		return *this;
	}

	FORCE_INLINE constexpr decltype(auto)
	operator*(auto v) const noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v * t{}; })
	{
		return t_this{ x * v, y * v };
	}

	FORCE_INLINE constexpr decltype(auto)
	operator*(const t_this& v) const noexcept
	{
		return t_this{ x * v.x, y * v.y };
	}

	FORCE_INLINE constexpr decltype(auto)
	operator/(auto v) const noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v / t{}; })
	{
		return t_this{ x / v, y / v };
	}

	FORCE_INLINE constexpr decltype(auto)
	operator/(const t_this& v) const noexcept
	{
		return t_this{ x / v.x, y / v.y };
	}

	FORCE_INLINE constexpr t_this&
	operator*=(auto v) noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v * t{}; })
	{
		x *= v;
		y *= v;
		return *this;
	}

	FORCE_INLINE constexpr t_this&
	operator/=(auto v) noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v / t{}; })
	{
		x /= v;
		y /= v;
		return *this;
	}

	FORCE_INLINE constexpr decltype(auto)
	operator-() const noexcept
		requires(std::is_signed_v<t>)

	{
		return t_this{ -x, -y };
	}

	FORCE_INLINE constexpr bool
	is_zero() const noexcept
	{
		return x == t{ 0 } && y == t{ 0 };
	}

	FORCE_INLINE constexpr t
	area() const noexcept
	{
		return t{ x * y };
	}

	FORCE_INLINE constexpr t
	manhattan() const noexcept
		requires(std::is_integral_v<t> && std::is_signed_v<t>)
	{
		auto ax = (x < 0) ? -x : x;
		auto ay = (y < 0) ? -y : y;
		return t{ ax + ay };
	}

	FORCE_INLINE static constexpr decltype(auto)
	zero() noexcept
	{
		return t_this{ 0, 0 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	one() noexcept
	{
		return t_this{ 1, 1 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	unit_x() noexcept
	{
		return t_this{ 1, 0 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	unit_y() noexcept
	{
		return t_this{ 0, 1 };
	}
};

template <typename t>
requires(std::is_arithmetic_v<t>)
struct vec3
{
	using t_value = t;
	using t_this  = vec3<t>;

	union
	{
		struct
		{
			t x, y;
		};

		vec2<t> xy;
	};

	t z;

	FORCE_INLINE constexpr vec3() noexcept = default;

	FORCE_INLINE constexpr vec3(auto&& other) noexcept
		requires(requires { other.x; other.y; other.z; }
				 and std::convertible_to<decltype(other.x), t>
				 and std::convertible_to<decltype(other.y), t>
				 and std::convertible_to<decltype(other.z), t>)
		: x{ static_cast<t>(FWD(other).x) }, y{ static_cast<t>(FWD(other).y) }, z{ static_cast<t>(FWD(other).z) }
	{
	}

	FORCE_INLINE constexpr vec3(auto&& other) noexcept
		requires(std::convertible_to<decltype(other), t>
				 and not requires { other.x; other.y; other.z; })
		: x{ static_cast<t>(FWD(other)) }, y{ static_cast<t>(FWD(other)) }, z{ static_cast<t>(FWD(other)) }
	{
	}

	FORCE_INLINE constexpr vec3(auto&& x_other, auto&& y_other, auto&& z_other) noexcept
		requires(std::convertible_to<decltype(x_other), t>
				 and std::convertible_to<decltype(y_other), t>
				 and std::convertible_to<decltype(z_other), t>)
		: x{ static_cast<t>(FWD(x_other)) }, y{ static_cast<t>(FWD(y_other)) }, z{ static_cast<t>(FWD(z_other)) }
	{
	}

	template <typename u>
	FORCE_INLINE constexpr vec3(const u (&other)[3]) noexcept
		requires(std::convertible_to<u, t>)
		: x{ static_cast<t>(other[0]) }, y{ static_cast<t>(other[1]) }, z{ static_cast<t>(other[2]) }
	{
	}

	FORCE_INLINE constexpr t*
	data() noexcept
	{
		return &x;
	}

	FORCE_INLINE constexpr const t*
	data() const noexcept
	{
		return &x;
	}

	FORCE_INLINE constexpr t&
	operator[](std::size_t c) noexcept
	{
		AGE_ASSERT(c < 3);
		return (&x)[c];
	}

	FORCE_INLINE constexpr const t&
	operator[](std::size_t c) const noexcept
	{
		AGE_ASSERT(c < 3);
		return (&x)[c];
	}

	FORCE_INLINE constexpr bool
	operator==(const t_this&) const noexcept = default;
	FORCE_INLINE constexpr auto
	operator<=>(const t_this&) const noexcept = default;

	FORCE_INLINE constexpr t_this
	operator+(const t_this& other_r) const noexcept
	{
		return { x + other_r.x, y + other_r.y, z + other_r.z };
	}

	FORCE_INLINE constexpr t_this
	operator-(const t_this& other_r) const noexcept
	{
		return { x - other_r.x, y - other_r.y, z - other_r.z };
	}

	FORCE_INLINE constexpr t_this&
	operator+=(const t_this& other_r) noexcept
	{
		x += other_r.x;
		y += other_r.y;
		z += other_r.z;
		return *this;
	}

	FORCE_INLINE constexpr t_this&
	operator-=(const t_this& other_r) noexcept
	{
		x -= other_r.x;
		y -= other_r.y;
		z -= other_r.z;
		return *this;
	}

	FORCE_INLINE constexpr decltype(auto)
	operator*(auto v) const noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v * t{}; })
	{
		return t_this{ x * v, y * v, z * v };
	}

	FORCE_INLINE constexpr decltype(auto)
	operator*(const t_this& v) const noexcept
	{
		return t_this{ x * v.x, y * v.y, z * v.z };
	}

	FORCE_INLINE constexpr decltype(auto)
	operator/(auto v) const noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v / t{}; })
	{
		return t_this{ x / v, y / v, z / v };
	}

	FORCE_INLINE constexpr decltype(auto)
	operator/(const t_this& v) const noexcept
	{
		return t_this{ x / v.x, y / v.y, z / v.z };
	}

	FORCE_INLINE constexpr t_this&
	operator*=(auto v) noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v * t{}; })
	{
		x *= v;
		y *= v;
		z *= v;
		return *this;
	}

	FORCE_INLINE constexpr t_this&
	operator/=(auto v) noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v / t{}; })
	{
		x /= v;
		y /= v;
		z /= v;
		return *this;
	}

	FORCE_INLINE constexpr decltype(auto)
	operator-() const noexcept
		requires(std::is_signed_v<t>)
	{
		return t_this{ -x, -y, -z };
	}

	FORCE_INLINE constexpr bool
	is_zero() const noexcept
	{
		return x == t{ 0 } && y == t{ 0 } && z == t{ 0 };
	}

	FORCE_INLINE constexpr t
	volume() const noexcept
	{
		return t{ x * y * z };
	}

	FORCE_INLINE constexpr t
	manhattan() const noexcept
		requires(std::is_integral_v<t> && std::is_signed_v<t>)
	{
		auto ax = (x < 0) ? -x : x;
		auto ay = (y < 0) ? -y : y;
		auto az = (z < 0) ? -z : z;
		return t{ ax + ay + az };
	}

	FORCE_INLINE static constexpr decltype(auto)
	zero() noexcept
	{
		return t_this{ 0, 0, 0 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	one() noexcept
	{
		return t_this{ 1, 1, 1 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	unit_x() noexcept
	{
		return t_this{ 1, 0, 0 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	unit_y() noexcept
	{
		return t_this{ 0, 1, 0 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	unit_z() noexcept
	{
		return t_this{ 0, 0, 1 };
	}
};

template <typename t>
requires(std::is_arithmetic_v<t>)
struct vec4
{
	using t_value = t;
	using t_this  = vec4<t>;

	union
	{
		struct
		{
			t x, y, z;
		};

		vec3<t> xyz;
	};

	t w;

	FORCE_INLINE constexpr vec4() noexcept = default;

	template <typename t_other>
	FORCE_INLINE constexpr vec4(t_other&& other) noexcept
		requires(
					requires { other.x; other.y; other.z; other.w; }
					&& std::convertible_to<decltype(other.x), t>
					&& std::convertible_to<decltype(other.y), t>
					&& std::convertible_to<decltype(other.z), t>
					&& std::convertible_to<decltype(other.w), t>)
		: xyz{ FWD(other).x, FWD(other).y, FWD(other).z }, w{ static_cast<t>(FWD(other).w) }
	{
	}

	template <typename t_other>
	FORCE_INLINE constexpr vec4(t_other&& other) noexcept
		requires(
					std::convertible_to<t_other, t>
					&& !requires { other.x; other.y; other.z; other.w; })
		: xyz{ FWD(other), FWD(other), FWD(other) }, w{ static_cast<t>(FWD(other)) }
	{
	}

	template <typename t_x, typename t_y, typename t_z, typename t_w>
	FORCE_INLINE constexpr vec4(t_x&& x_other, t_y&& y_other, t_z&& z_other, t_w&& w_other) noexcept
		requires(
					std::convertible_to<t_x, t>
					&& std::convertible_to<t_y, t>
					&& std::convertible_to<t_z, t>
					&& std::convertible_to<t_w, t>)
		: xyz{ FWD(x_other), FWD(y_other), FWD(z_other) }, w{ static_cast<t>(FWD(w_other)) }
	{
	}

	template <typename u>
	FORCE_INLINE constexpr vec4(const u (&other)[4]) noexcept
		requires(std::convertible_to<u, t>)
		: x{ static_cast<t>(other[0]) }, y{ static_cast<t>(other[1]) }, z{ static_cast<t>(other[2]) }, w{ static_cast<t>(other[3]) }
	{
	}

	FORCE_INLINE constexpr t*
	data() noexcept
	{
		return &x;
	}

	FORCE_INLINE constexpr const t*
	data() const noexcept
	{
		return &x;
	}

	FORCE_INLINE constexpr t&
	operator[](std::size_t c) noexcept
	{
		AGE_ASSERT(c < 4);
		return (&x)[c];
	}

	FORCE_INLINE constexpr const t&
	operator[](std::size_t c) const noexcept
	{
		AGE_ASSERT(c < 4);
		return (&x)[c];
	}

	FORCE_INLINE constexpr bool
	operator==(const t_this&) const noexcept = default;

	FORCE_INLINE constexpr auto
	operator<=>(const t_this&) const noexcept = default;

	FORCE_INLINE constexpr t_this
	operator+(const t_this& other_r) const noexcept
	{
		return { x + other_r.x, y + other_r.y, z + other_r.z, w + other_r.w };
	}

	FORCE_INLINE constexpr t_this
	operator-(const t_this& other_r) const noexcept
	{
		return { x - other_r.x, y - other_r.y, z - other_r.z, w - other_r.w };
	}

	FORCE_INLINE constexpr t_this&
	operator+=(const t_this& other_r) noexcept
	{
		x += other_r.x;
		y += other_r.y;
		z += other_r.z;
		w += other_r.w;
		return *this;
	}

	FORCE_INLINE constexpr t_this&
	operator-=(const t_this& other_r) noexcept
	{
		x -= other_r.x;
		y -= other_r.y;
		z -= other_r.z;
		w -= other_r.w;
		return *this;
	}

	FORCE_INLINE constexpr decltype(auto)
	operator*(auto v) const noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v * t{}; })
	{
		return t_this{ x * v, y * v, z * v, w * v };
	}

	FORCE_INLINE constexpr decltype(auto)
	operator*(const t_this& v) const noexcept
	{
		return t_this{ x * v.x, y * v.y, z * v.z, w * v.w };
	}

	FORCE_INLINE constexpr decltype(auto)
	operator/(auto v) const noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v / t{}; })
	{
		return t_this{ x / v, y / v, z / v, w / v };
	}

	FORCE_INLINE constexpr decltype(auto)
	operator/(const t_this& v) const noexcept
	{
		return t_this{ x / v.x, y / v.y, z / v.z, w / v.w };
	}

	FORCE_INLINE constexpr t_this&
	operator*=(auto v) noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v * t{}; })
	{
		x *= v;
		y *= v;
		z *= v;
		w *= v;
		return *this;
	}

	FORCE_INLINE constexpr t_this&
	operator/=(auto v) noexcept
		requires(std::is_arithmetic_v<std::decay_t<decltype(v)>> and requires { v / t{}; })
	{
		x /= v;
		y /= v;
		z /= v;
		w /= v;
		return *this;
	}

	FORCE_INLINE constexpr decltype(auto)
	operator-() const noexcept
		requires(std::is_signed_v<t>)
	{
		return t_this{ -x, -y, -z, -w };
	}

	FORCE_INLINE constexpr bool
	is_zero() const noexcept
	{
		return x == t{ 0 } && y == t{ 0 } && z == t{ 0 } && w == t{ 0 };
	}

	FORCE_INLINE constexpr t
	manhattan() const noexcept
		requires(std::is_integral_v<t> && std::is_signed_v<t>)
	{
		auto ax = (x < 0) ? -x : x;
		auto ay = (y < 0) ? -y : y;
		auto az = (z < 0) ? -z : z;
		auto aw = (w < 0) ? -w : w;
		return t{ ax + ay + az + aw };
	}

	FORCE_INLINE static constexpr decltype(auto)
	zero() noexcept
	{
		return t_this{ 0, 0, 0, 0 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	one() noexcept
	{
		return t_this{ 1, 1, 1, 1 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	unit_x() noexcept
	{
		return t_this{ 1, 0, 0, 0 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	unit_y() noexcept
	{
		return t_this{ 0, 1, 0, 0 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	unit_z() noexcept
	{
		return t_this{ 0, 0, 1, 0 };
	}

	FORCE_INLINE static constexpr decltype(auto)
	unit_w() noexcept
	{
		return t_this{ 0, 0, 0, 1 };
	}
};

template <typename t>
requires(std::is_arithmetic_v<t>)
struct alignas(16) vec2a : public vec2<t>
{
	using t_base = vec2<t>;
	using t_base::t_base;
};

template <typename t>
requires(std::is_arithmetic_v<t>)
struct alignas(16) vec3a : public vec3<t>
{
	using t_base = vec3<t>;
	using t_base::t_base;
};

template <typename t>
requires(std::is_arithmetic_v<t>)
struct alignas(16) vec4a : public vec4<t>
{
	using t_base = vec4<t>;
	using t_base::t_base;
};

template <typename t>
requires(std::is_arithmetic_v<t>)
struct mat22
{
	using t_value = t;
	using t_this  = mat22<t>;
	using t_row	  = vec2<t>;

	t_row r0{ t{ 1 }, t{ 0 } };
	t_row r1{ t{ 0 }, t{ 1 } };

	FORCE_INLINE constexpr mat22() noexcept = default;

	FORCE_INLINE constexpr mat22(auto&& other) noexcept
		requires(std::convertible_to<decltype(other), t_row>)
		: r0{ FWD(other) }, r1{ FWD(other) }
	{
	}

	FORCE_INLINE constexpr mat22(auto&& other) noexcept
		requires(not std::convertible_to<decltype(other), t_row>
				 and requires { other.r0; other.r1; }
				 and std::convertible_to<decltype(other.r0), t_row>
				 and std::convertible_to<decltype(other.r1), t_row>)
		: r0{ FWD(other).r0 }, r1{ FWD(other).r1 }
	{
	}

	FORCE_INLINE constexpr mat22(auto&& other1, auto&& other2) noexcept
		requires(std::convertible_to<decltype(other1), t_row>
				 and std::convertible_to<decltype(other2), t_row>)
		: r0{ FWD(other1) }, r1{ FWD(other2) }
	{
	}

	FORCE_INLINE constexpr t*
	data() noexcept
	{
		return &r0[0];
	}

	FORCE_INLINE constexpr const t*
	data() const noexcept
	{
		return &r0[0];
	}

	FORCE_INLINE constexpr t_row&
	operator[](std::size_t r) noexcept
	{
		return (&r0)[r];
	}

	FORCE_INLINE constexpr const t_row&
	operator[](std::size_t r) const noexcept
	{
		return (&r0)[r];
	}

	FORCE_INLINE constexpr decltype(auto)
	col(std::size_t c) const noexcept
	{
		return t_row{ r0[c], r1[c] };
	}

	FORCE_INLINE constexpr void
	transpose() noexcept
	{
		auto temp	  = (*this)[0][1];
		(*this)[0][1] = (*this)[1][0];
		(*this)[1][0] = temp;
	}

	FORCE_INLINE static constexpr decltype(auto)
	identity() noexcept
	{
		return t_this{};
	}

	FORCE_INLINE static constexpr decltype(auto)
	zero() noexcept
	{
		return t_this{ t{ 0 } };
	}

	FORCE_INLINE static constexpr decltype(auto)
	from_diagonal(t v) noexcept
	{
		return t_this{ t_row{ t{ v }, t{ 0 } },
					   t_row{ t{ 0 }, t{ v } } };
	}

	FORCE_INLINE static constexpr std::size_t
	dim() noexcept
	{
		return 2;
	}

	FORCE_INLINE static constexpr std::size_t
	rows() noexcept
	{
		return dim();
	}

	FORCE_INLINE static constexpr std::size_t
	cols() noexcept
	{
		return dim();
	}
};

template <typename t>
requires(std::is_arithmetic_v<t>)
struct mat33
{
	using t_value = t;
	using t_this  = mat33<t>;
	using t_row	  = vec3<t>;

	t_row r0{ t{ 1 }, t{ 0 }, t{ 0 } };
	t_row r1{ t{ 0 }, t{ 1 }, t{ 0 } };
	t_row r2{ t{ 0 }, t{ 0 }, t{ 1 } };

	FORCE_INLINE constexpr mat33() noexcept = default;

	FORCE_INLINE constexpr mat33(auto&& other) noexcept
		requires(std::convertible_to<decltype(other), t_row>)
		: r0{ FWD(other) }, r1{ FWD(other) }, r2{ FWD(other) }
	{
	}

	FORCE_INLINE constexpr mat33(auto&& other) noexcept
		requires(not std::convertible_to<decltype(other), t_row>
				 and requires { other.r0; other.r1; other.r2; }
				 and std::convertible_to<decltype(other.r0), t_row>
				 and std::convertible_to<decltype(other.r1), t_row>
				 and std::convertible_to<decltype(other.r2), t_row>)
		: r0{ FWD(other).r0 }, r1{ FWD(other).r1 }, r2{ FWD(other).r2 }
	{
	}

	FORCE_INLINE constexpr mat33(auto&& other1, auto&& other2, auto&& other3) noexcept
		requires(std::convertible_to<decltype(other1), t_row>
				 and std::convertible_to<decltype(other2), t_row>
				 and std::convertible_to<decltype(other3), t_row>)
		: r0{ FWD(other1) }, r1{ FWD(other2) }, r2{ FWD(other3) }
	{
	}

	FORCE_INLINE constexpr t*
	data() noexcept
	{
		return &r0[0];
	}

	FORCE_INLINE constexpr const t*
	data() const noexcept
	{
		return &r0[0];
	}

	FORCE_INLINE constexpr t_row&
	operator[](std::size_t r) noexcept
	{
		return (&r0)[r];
	}

	FORCE_INLINE constexpr const t_row&
	operator[](std::size_t r) const noexcept
	{
		return (&r0)[r];
	}

	FORCE_INLINE constexpr decltype(auto)
	col(std::size_t c) const noexcept
	{
		return t_row{ r0[c], r1[c], r2[c] };
	}

	FORCE_INLINE constexpr void
	transpose() noexcept
	{
		// swap (0,1), (0,2), (1,2)
		auto temp	  = (*this)[0][1];
		(*this)[0][1] = (*this)[1][0];
		(*this)[1][0] = temp;

		temp		  = (*this)[0][2];
		(*this)[0][2] = (*this)[2][0];
		(*this)[2][0] = temp;

		temp		  = (*this)[1][2];
		(*this)[1][2] = (*this)[2][1];
		(*this)[2][1] = temp;
	}

	FORCE_INLINE static constexpr decltype(auto)
	identity() noexcept
	{
		return t_this{};
	}

	FORCE_INLINE static constexpr decltype(auto)
	zero() noexcept
	{
		return t_this{ t{ 0 } };
	}

	FORCE_INLINE static constexpr decltype(auto)
	from_diagonal(t v) noexcept
	{
		return t_this{
			t_row{ t{ v }, t{ 0 }, t{ 0 } },
			t_row{ t{ 0 }, t{ v }, t{ 0 } },
			t_row{ t{ 0 }, t{ 0 }, t{ v } },
		};
	}

	FORCE_INLINE static constexpr std::size_t
	dim() noexcept
	{
		return 3;
	}

	FORCE_INLINE static constexpr std::size_t
	rows() noexcept
	{
		return dim();
	}

	FORCE_INLINE static constexpr std::size_t
	cols() noexcept
	{
		return dim();
	}
};

template <typename t>
requires(std::is_arithmetic_v<t>)
struct mat44
{
	using t_value = t;
	using t_this  = mat44<t>;
	using t_row	  = vec4<t>;

	t_row r0{ t{ 1 }, t{ 0 }, t{ 0 }, t{ 0 } };
	t_row r1{ t{ 0 }, t{ 1 }, t{ 0 }, t{ 0 } };
	t_row r2{ t{ 0 }, t{ 0 }, t{ 1 }, t{ 0 } };
	t_row r3{ t{ 0 }, t{ 0 }, t{ 0 }, t{ 1 } };

	FORCE_INLINE constexpr mat44() noexcept = default;

	FORCE_INLINE constexpr mat44(auto&& other) noexcept
		requires(std::convertible_to<decltype(other), t_row>)
		: r0{ FWD(other) }, r1{ FWD(other) }, r2{ FWD(other) }, r3{ FWD(other) }
	{
	}

	FORCE_INLINE constexpr mat44(auto&& other) noexcept
		requires(not std::convertible_to<decltype(other), t_row>
				 and requires { other.r0; other.r1; other.r2; other.r3; }
				 and std::convertible_to<decltype(other.r0), t_row>
				 and std::convertible_to<decltype(other.r1), t_row>
				 and std::convertible_to<decltype(other.r2), t_row>
				 and std::convertible_to<decltype(other.r3), t_row>)
		: r0{ FWD(other).r0 }, r1{ FWD(other).r1 }, r2{ FWD(other).r2 }, r3{ FWD(other).r3 }
	{
	}

	FORCE_INLINE constexpr mat44(auto&& other1, auto&& other2, auto&& other3, auto&& other4) noexcept
		requires(std::convertible_to<decltype(other1), t_row>
				 and std::convertible_to<decltype(other2), t_row>
				 and std::convertible_to<decltype(other3), t_row>
				 and std::convertible_to<decltype(other4), t_row>)
		: r0{ FWD(other1) }, r1{ FWD(other2) }, r2{ FWD(other3) }, r3{ FWD(other4) }
	{
	}

	FORCE_INLINE constexpr t*
	data() noexcept
	{
		return &r0[0];
	}

	FORCE_INLINE constexpr const t*
	data() const noexcept
	{
		return &r0[0];
	}

	FORCE_INLINE constexpr t_row&
	operator[](std::size_t r) noexcept
	{
		return (&r0)[r];
	}

	FORCE_INLINE constexpr const t_row&
	operator[](std::size_t r) const noexcept
	{
		return (&r0)[r];
	}

	FORCE_INLINE constexpr decltype(auto)
	col(std::size_t c) const noexcept
	{
		return t_row{ r0[c], r1[c], r2[c], r3[c] };
	}

	FORCE_INLINE constexpr void
	transpose() noexcept
	{
		// swap upper triangle with lower triangle: 6 swaps
		auto temp	  = (*this)[0][1];
		(*this)[0][1] = (*this)[1][0];
		(*this)[1][0] = temp;

		temp		  = (*this)[0][2];
		(*this)[0][2] = (*this)[2][0];
		(*this)[2][0] = temp;

		temp		  = (*this)[0][3];
		(*this)[0][3] = (*this)[3][0];
		(*this)[3][0] = temp;

		temp		  = (*this)[1][2];
		(*this)[1][2] = (*this)[2][1];
		(*this)[2][1] = temp;

		temp		  = (*this)[1][3];
		(*this)[1][3] = (*this)[3][1];
		(*this)[3][1] = temp;

		temp		  = (*this)[2][3];
		(*this)[2][3] = (*this)[3][2];
		(*this)[3][2] = temp;
	}

	FORCE_INLINE static constexpr decltype(auto)
	identity() noexcept
	{
		return t_this{};
	}

	FORCE_INLINE static constexpr decltype(auto)
	zero() noexcept
	{
		return t_this{ t{ 0 } };
	}

	FORCE_INLINE static constexpr decltype(auto)
	from_diagonal(t v) noexcept
	{
		return t_this{
			t_row{ t{ v }, t{ 0 }, t{ 0 }, t{ 0 } },
			t_row{ t{ 0 }, t{ v }, t{ 0 }, t{ 0 } },
			t_row{ t{ 0 }, t{ 0 }, t{ v }, t{ 0 } },
			t_row{ t{ 0 }, t{ 0 }, t{ 0 }, t{ v } },
		};
	}

	FORCE_INLINE static constexpr std::size_t
	dim() noexcept
	{
		return 4;
	}

	FORCE_INLINE static constexpr std::size_t
	rows() noexcept
	{
		return dim();
	}

	FORCE_INLINE static constexpr std::size_t
	cols() noexcept
	{
		return dim();
	}
};

template <typename t>
requires(std::is_arithmetic_v<t>)
struct alignas(16) mat22a : public mat22<t>
{
	using t_base = mat22<t>;
	using t_base::t_base;
};

template <typename t>
requires(std::is_arithmetic_v<t>)
struct alignas(16) mat33a : public mat33<t>
{
	using t_base = mat33<t>;
	using t_base::t_base;
};

template <typename t>
requires(std::is_arithmetic_v<t>)
struct alignas(16) mat44a : public mat44<t>
{
	using t_base = mat44<t>;
	using t_base::t_base;
};

using uint8_2 = vec2<uint8>;
using uint8_3 = vec3<uint8>;
using uint8_4 = vec4<uint8>;

using uint16_2 = vec2<uint16>;
using uint16_3 = vec3<uint16>;
using uint16_4 = vec4<uint16>;

using uint32_2 = vec2<uint32>;
using uint32_3 = vec3<uint32>;
using uint32_4 = vec4<uint32>;

using uint64_2 = vec2<uint64>;
using uint64_3 = vec3<uint64>;
using uint64_4 = vec4<uint64>;

using int8_2 = vec2<int8>;
using int8_3 = vec3<int8>;
using int8_4 = vec4<int8>;

using int16_2 = vec2<int16>;
using int16_3 = vec3<int16>;
using int16_4 = vec4<int16>;

using int32_2 = vec2<int32>;
using int32_3 = vec3<int32>;
using int32_4 = vec4<int32>;

using int64_2 = vec2<int64>;
using int64_3 = vec3<int64>;
using int64_4 = vec4<int64>;

using float2 = vec2<float>;
using float3 = vec3<float>;
using float4 = vec4<float>;

using double2 = vec2<double>;
using double3 = vec3<double>;
using double4 = vec4<double>;

using uint8_2a = vec2a<uint8>;
using uint8_3a = vec3a<uint8>;
using uint8_4a = vec4a<uint8>;

using uint16_2a = vec2a<uint16>;
using uint16_3a = vec3a<uint16>;
using uint16_4a = vec4a<uint16>;

using uint32_2a = vec2a<uint32>;
using uint32_3a = vec3a<uint32>;
using uint32_4a = vec4a<uint32>;

using uint64_2a = vec2a<uint64>;
using uint64_3a = vec3a<uint64>;
using uint64_4a = vec4a<uint64>;

using int8_2a = vec2a<int8>;
using int8_3a = vec3a<int8>;
using int8_4a = vec4a<int8>;

using int16_2a = vec2a<int16>;
using int16_3a = vec3a<int16>;
using int16_4a = vec4a<int16>;

using int32_2a = vec2a<int32>;
using int32_3a = vec3a<int32>;
using int32_4a = vec4a<int32>;

using int64_2a = vec2a<int64>;
using int64_3a = vec3a<int64>;
using int64_4a = vec4a<int64>;

using float2a = vec2a<float>;
using float3a = vec3a<float>;
using float4a = vec4a<float>;

using double2a = vec2a<double>;
using double3a = vec3a<double>;
using double4a = vec4a<double>;

using float2x2 = mat22<float>;
using float3x3 = mat33<float>;
using float4x4 = mat44<float>;

using float2x2a = mat22a<float>;
using float3x3a = mat33a<float>;
using float4x4a = mat44a<float>;

using double2x2 = mat22<double>;
using double3x3 = mat33<double>;
using double4x4 = mat44<double>;

using double2x2a = mat22a<double>;
using double3x3a = mat33a<double>;
using double4x4a = mat44a<double>;

using half2 = vec2<half>;
using half3 = vec3<half>;
using half4 = vec4<half>;

static_assert(alignof(uint8_2a) == 16 and sizeof(uint8_2a) % 16 == 0);
static_assert(alignof(uint8_3a) == 16 and sizeof(uint8_3a) % 16 == 0);
static_assert(alignof(uint8_4a) == 16 and sizeof(uint8_4a) % 16 == 0);
static_assert(alignof(uint16_2a) == 16 and sizeof(uint16_2a) % 16 == 0);
static_assert(alignof(uint16_3a) == 16 and sizeof(uint16_3a) % 16 == 0);
static_assert(alignof(uint16_4a) == 16 and sizeof(uint16_4a) % 16 == 0);
static_assert(alignof(uint32_2a) == 16 and sizeof(uint32_2a) % 16 == 0);
static_assert(alignof(uint32_3a) == 16 and sizeof(uint32_3a) % 16 == 0);
static_assert(alignof(uint32_4a) == 16 and sizeof(uint32_4a) % 16 == 0);
static_assert(alignof(uint64_2a) == 16 and sizeof(uint64_2a) % 16 == 0);
static_assert(alignof(uint64_3a) == 16 and sizeof(uint64_3a) % 16 == 0);
static_assert(alignof(uint64_4a) == 16 and sizeof(uint64_4a) % 16 == 0);
static_assert(alignof(int8_2a) == 16 and sizeof(int8_2a) % 16 == 0);
static_assert(alignof(int8_3a) == 16 and sizeof(int8_3a) % 16 == 0);
static_assert(alignof(int8_4a) == 16 and sizeof(int8_4a) % 16 == 0);
static_assert(alignof(int16_2a) == 16 and sizeof(int16_2a) % 16 == 0);
static_assert(alignof(int16_3a) == 16 and sizeof(int16_3a) % 16 == 0);
static_assert(alignof(int16_4a) == 16 and sizeof(int16_4a) % 16 == 0);
static_assert(alignof(int32_2a) == 16 and sizeof(int32_2a) % 16 == 0);
static_assert(alignof(int32_3a) == 16 and sizeof(int32_3a) % 16 == 0);
static_assert(alignof(int32_4a) == 16 and sizeof(int32_4a) % 16 == 0);
static_assert(alignof(int64_2a) == 16 and sizeof(int64_2a) % 16 == 0);
static_assert(alignof(int64_3a) == 16 and sizeof(int64_3a) % 16 == 0);
static_assert(alignof(int64_4a) == 16 and sizeof(int64_4a) % 16 == 0);
static_assert(alignof(float2a) == 16 and sizeof(float2a) % 16 == 0);
static_assert(alignof(float3a) == 16 and sizeof(float3a) % 16 == 0);
static_assert(alignof(float4a) == 16 and sizeof(float4a) % 16 == 0);
static_assert(alignof(double2a) == 16 and sizeof(double2a) % 16 == 0);
static_assert(alignof(double3a) == 16 and sizeof(double3a) % 16 == 0);
static_assert(alignof(double4a) == 16 and sizeof(double4a) % 16 == 0);

static_assert(alignof(float2x2a) == 16 and sizeof(float2x2a) % 16 == 0);
static_assert(alignof(float3x3a) == 16 and sizeof(float3x3a) % 16 == 0);
static_assert(alignof(float4x4a) == 16 and sizeof(float4x4a) % 16 == 0);
static_assert(alignof(double2x2a) == 16 and sizeof(double2x2a) % 16 == 0);
static_assert(alignof(double3x3a) == 16 and sizeof(double3x3a) % 16 == 0);
static_assert(alignof(double4x4a) == 16 and sizeof(double4x4a) % 16 == 0);

static_assert(offsetof(float2x2, r1) == sizeof(float2x2::t_row) * (float2x2::dim() - 1));
static_assert(offsetof(float3x3, r2) == sizeof(float3x3::t_row) * (float3x3::dim() - 1));
static_assert(offsetof(float4x4, r3) == sizeof(float4x4::t_row) * (float4x4::dim() - 1));
static_assert(offsetof(float2x2a, r1) == sizeof(float2x2a::t_row) * (float2x2a::dim() - 1));
static_assert(offsetof(float3x3a, r2) == sizeof(float3x3a::t_row) * (float3x3a::dim() - 1));
static_assert(offsetof(float4x4a, r3) == sizeof(float4x4a::t_row) * (float4x4a::dim() - 1));

static_assert(offsetof(double2x2, r1) == sizeof(double2x2::t_row) * (double2x2::dim() - 1));
static_assert(offsetof(double3x3, r2) == sizeof(double3x3::t_row) * (double3x3::dim() - 1));
static_assert(offsetof(double4x4, r3) == sizeof(double4x4::t_row) * (double4x4::dim() - 1));
static_assert(offsetof(double2x2a, r1) == sizeof(double2x2a::t_row) * (double2x2a::dim() - 1));
static_assert(offsetof(double3x3a, r2) == sizeof(double3x3a::t_row) * (double3x3a::dim() - 1));
static_assert(offsetof(double4x4a, r3) == sizeof(double4x4a::t_row) * (double4x4a::dim() - 1));

// template <typename t>
// struct oct;

template <typename t>
requires std::is_same_v<t, uint8> or std::is_same_v<t, int8>
struct oct
{
	using t_value = t;
	t x;
	t y;
};

namespace age::inline math
{
	template <typename t>
	FORCE_INLINE constexpr bool
	is_zero(const t& v) noexcept
	{
		if constexpr (std::is_floating_point_v<std::remove_cvref_t<t>>)
		{
			return std::abs(v) <= std::numeric_limits<std::remove_cvref_t<t>>::epsilon();
		}
		else
		{
			return v == t{ 0 };
		}
	}

	template <typename t>
	FORCE_INLINE constexpr bool
	is_zero(const vec2<t>& v) noexcept
	{
		return is_zero(v.x) and is_zero(v.y);
	}

	template <typename t>
	FORCE_INLINE constexpr bool
	is_zero(const vec3<t>& v) noexcept
	{
		return is_zero(v.x) and is_zero(v.y) and is_zero(v.z);
	}

	template <typename t>
	FORCE_INLINE constexpr bool
	is_zero(const vec4<t>& v) noexcept
	{
		return is_zero(v.x) and is_zero(v.y) and is_zero(v.z) and is_zero(v.w);
	}

	template <typename t>
	constexpr float
	dot(const vec2<t>& v_l, const vec2<t>& v_r) noexcept
	{
		return v_l.x * v_r.x + v_l.y * v_r.y;
	}

	template <typename t>
	constexpr float
	dot(const vec3<t>& v_l, const vec3<t>& v_r) noexcept
	{
		return v_l.x * v_r.x + v_l.y * v_r.y + v_l.z * v_r.z;
	}

	template <typename t>
	constexpr float
	dot(const vec4<t>& v_l, const vec4<t>& v_r) noexcept
	{
		return v_l.x * v_r.x + v_l.y * v_r.y + v_l.z * v_r.z + v_l.w * v_r.w;
	}
}	 // namespace age::inline math

// math utils

namespace age::inline math
{
	FORCE_INLINE constexpr decltype(auto)
	ceil(auto&& vec) noexcept
	{
		if constexpr (std::is_same_v<BARE_OF(vec), float2> or std::is_same_v<BARE_OF(vec), float2a>)
		{
			return float2{ std::ceil(vec.x), std::ceil(vec.y) };
		}
		else if constexpr (std::is_same_v<BARE_OF(vec), float3> or std::is_same_v<BARE_OF(vec), float3a>)
		{
			return float3{ std::ceil(vec.x), std::ceil(vec.y), std::ceil(vec.z) };
		}
		else if constexpr (std::is_same_v<BARE_OF(vec), float4> or std::is_same_v<BARE_OF(vec), float4a>)
		{
			return float4{ std::ceil(vec.x), std::ceil(vec.y), std::ceil(vec.z), std::ceil(vec.w) };
		}
		else
		{
			static_assert(false, "unsupported type for ceil");
		}
	}

	FORCE_INLINE constexpr decltype(auto)
	floor(auto&& vec) noexcept
	{
		if constexpr (std::is_same_v<BARE_OF(vec), float2> or std::is_same_v<BARE_OF(vec), float2a>)
		{
			return float2{ std::floor(vec.x), std::floor(vec.y) };
		}
		else if constexpr (std::is_same_v<BARE_OF(vec), float3> or std::is_same_v<BARE_OF(vec), float3a>)
		{
			return float3{ std::floor(vec.x), std::floor(vec.y), std::floor(vec.z) };
		}
		else if constexpr (std::is_same_v<BARE_OF(vec), float4> or std::is_same_v<BARE_OF(vec), float4a>)
		{
			return float4{ std::floor(vec.x), std::floor(vec.y), std::floor(vec.z), std::floor(vec.w) };
		}
		else
		{
			static_assert(false, "unsupported type for floor");
		}
	}

	template <template <typename> typename v, typename t>
	FORCE_INLINE constexpr decltype(auto)
	min(const v<t>& lhs, const v<t>& rhs) noexcept
	{
		if constexpr (std::is_same_v<v<t>, float2> or std::is_same_v<v<t>, float2a>)
		{
			return v<t>{ std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y) };
		}
		else if constexpr (std::is_same_v<v<t>, float3> or std::is_same_v<v<t>, float3a>)
		{
			return v<t>{ std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y), std::min(lhs.z, rhs.z) };
		}
		else if constexpr (std::is_same_v<v<t>, float4> or std::is_same_v<v<t>, float4a>)
		{
			return v<t>{ std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y), std::min(lhs.z, rhs.z), std::min(lhs.w, rhs.w) };
		}
		else
		{
			static_assert(false, "unsupported type for floor");
		}
	}

	template <template <typename> typename v, typename t>
	FORCE_INLINE constexpr decltype(auto)
	max(const v<t>& lhs, const v<t>& rhs) noexcept
	{
		if constexpr (std::is_same_v<v<t>, float2> or std::is_same_v<v<t>, float2a>)
		{
			return v<t>{ std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y) };
		}
		else if constexpr (std::is_same_v<v<t>, float3> or std::is_same_v<v<t>, float3a>)
		{
			return v<t>{ std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y), std::max(lhs.z, rhs.z) };
		}
		else if constexpr (std::is_same_v<v<t>, float4> or std::is_same_v<v<t>, float4a>)
		{
			return v<t>{ std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y), std::max(lhs.z, rhs.z), std::max(lhs.w, rhs.w) };
		}
		else
		{
			static_assert(false, "unsupported type for floor");
		}
	}

	template <template <typename> typename v, typename t_>
	FORCE_INLINE constexpr decltype(auto)
	lerp(const v<t_>& a, const v<t_>& b, float t) noexcept
	{
		if constexpr (std::is_same_v<v<t_>, float2> or std::is_same_v<v<t_>, float2a>)
		{
			return v<t_>{ std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t) };
		}
		else if constexpr (std::is_same_v<v<t_>, float3> or std::is_same_v<v<t_>, float3a>)
		{
			return v<t_>{ std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t) };
		}
		else if constexpr (std::is_same_v<v<t_>, float4> or std::is_same_v<v<t_>, float4a>)
		{
			return v<t_>{ std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t), std::lerp(a.w, b.w, t) };
		}
		else
		{
			static_assert(false, "unsupported type for lerp");
		}
	}

	FORCE_INLINE constexpr decltype(auto)
	lerp(float a, float b, float t) noexcept
	{
		return a + t * (b - a);
	}
}	 // namespace age::inline math

namespace age::inline math::g
{
	inline constexpr auto sqrt_2	 = 1.4142135623730950488f;
	inline constexpr auto sqrt_2_inv = 0.7071067811865475244f;

	inline constexpr auto quaternion_identity = float4{ 0.f, 0.f, 0.f, 1.f };

	inline constexpr auto fsign_mask = 0x8000'0000u;

	// 0.0001
	inline constexpr auto epsilon_1e4 = 1e-4f;
	inline constexpr auto epsilon_1e6 = 1e-6f;

	inline constexpr auto pi			   = 3.141592654f;
	inline constexpr auto pi_2			   = 6.283185307f;
	inline constexpr auto pi_inv		   = 0.318309886f;
	inline constexpr auto pi_2_inv		   = 0.159154943f;
	inline constexpr auto pi_div_2		   = 1.570796327f;
	inline constexpr auto pi_half		   = pi_div_2;
	inline constexpr auto pi_div_4		   = 0.785398163f;
	inline constexpr auto degree_to_radian = pi / 180.f;
	inline constexpr auto radian_to_degree = 180.f / pi;

}	 // namespace age::inline math::g