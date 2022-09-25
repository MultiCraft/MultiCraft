/*
Minetest
Copyright (C) 2021  rubenwardy

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include "debug.h"

struct nullopt_t
{
};
constexpr nullopt_t nullopt{};

/**
 * An implementation of optional for C++11, which aims to be
 * compatible with a subset of std::optional features.
 *
 * Unfortunately, Minetest doesn't use C++17 yet.
 *
 * @tparam T The type to be stored
 */
template <typename T>
class Optional
{
	bool m_has_value = false;
	T m_value;

public:
	Optional() noexcept {}
	Optional(nullopt_t) noexcept {}
	Optional(const T &value) noexcept : m_has_value(true), m_value(value) {}
	Optional(const Optional<T> &other) noexcept :
			m_has_value(other.m_has_value), m_value(other.m_value)
	{
	}

	void operator=(nullopt_t) noexcept { m_has_value = false; }

	void operator=(const Optional<T> &other) noexcept
	{
		m_has_value = other.m_has_value;
		m_value = other.m_value;
	}

	T &value()
	{
		FATAL_ERROR_IF(!m_has_value, "optional doesn't have value");
		return m_value;
	}

	const T &value() const
	{
		FATAL_ERROR_IF(!m_has_value, "optional doesn't have value");
		return m_value;
	}

	const T &value_or(const T &def) const { return m_has_value ? m_value : def; }

	bool has_value() const noexcept { return m_has_value; }

	explicit operator bool() const { return m_has_value; }
};
