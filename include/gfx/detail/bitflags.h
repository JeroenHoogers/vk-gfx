// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <type_traits>

#define ENABLE_BITMASK_OPERATORS(EnumType) \
	template <>                            \
	struct enable_bitmask_operators<EnumType> : std::true

namespace gfx
{
	namespace detail
	{
		template <typename E>
		struct enable_bitmask_operators : std::false_type
		{
		};

		template <typename E>
		concept BitmaskEnum = std::is_enum_v<E> && enable_bitmask_operators<E>::value;

	} // namespace detail

	template <detail::BitmaskEnum E>
	constexpr E operator|(E lhs, E rhs) noexcept
	{
		using U = std::underlying_type_t<E>;

		return static_cast<E>(
			static_cast<U>(lhs) | static_cast<U>(rhs)
		);
	}

	template <detail::BitmaskEnum E>
	constexpr E operator&(E lhs, E rhs) noexcept
	{
		using U = std::underlying_type_t<E>;

		return static_cast<E>(
			static_cast<U>(lhs) & static_cast<U>(rhs)
		);
	}

	template <detail::BitmaskEnum E>
	constexpr E operator^(E lhs, E rhs) noexcept
	{
		using U = std::underlying_type_t<E>;

		return static_cast<E>(
			static_cast<U>(lhs) ^ static_cast<U>(rhs)
		);
	}

	template <detail::BitmaskEnum E>
	constexpr E operator~(E value) noexcept
	{
		using U = std::underlying_type_t<E>;

		return static_cast<E>(
			~static_cast<U>(value)
		);
	}

	template <detail::BitmaskEnum E>
	constexpr E& operator|=(E& lhs, E rhs) noexcept
	{
		return lhs = lhs | rhs;
	}

	template <detail::BitmaskEnum E>
	constexpr E& operator&=(E& lhs, E rhs) noexcept
	{
		return lhs = lhs & rhs;
	}

	template <detail::BitmaskEnum E>
	constexpr E& operator^=(E& lhs, E rhs) noexcept
	{
		return lhs = lhs ^ rhs;
	}
} // namespace gfx
