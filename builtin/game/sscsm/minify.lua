--
-- A primitive code minifier
--
-- Copyright © 2019-2021 by luk3yx
-- Copyright © 2020-2021 MultiCraft Development Team
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 3.0 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program; if not, write to the Free Software Foundation,
-- Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
--

-- Find multiple patterns
local function find_multiple(text, ...)
	local n = select('#', ...)
	local s, e, pattern
	for i = 1, n do
		local p = select(i, ...)
		local s2, e2 = text:find(p)
		if s2 and (not s or s2 < s) then
			s, e, pattern = s2, e2 or s2, p
		end
	end
	return s, e, pattern
end

-- Matches
-- These take 2-3 arguments (code, res, char) and should return code and res.
local matches = {
	-- Handle multi-line strings
	['%[=*%['] = function(code, res, char)
		res = res .. char
		char = char:sub(2, -2)
		local s, e = code:find(']' .. char .. ']', nil, true)
		if not s or not e then return code, res end
		return code:sub(e + 1), res .. code:sub(1, e)
	end,

	-- Handle regular comments
	['--'] = function(code, res, char)
		local s, e = code:find('\n', nil, true)
		if not s or not e then return '', res end

		-- Don't remove copyright or license information.
		if e >= 7 then
			local first_word = (code:match('^[ \t]*(%w+)') or ''):lower()
			if first_word == 'copyright' or first_word == 'license' then
				return code:sub(s), res .. char .. code:sub(1, s - 1)
			end
		end

		-- Shift trailing spaces back
		local spaces = res:match('(%s*)$') or ''
		return spaces .. code:sub(s), res:sub(1, #res - #spaces)
	end,

	-- Handle multi-line comments
	['%-%-%[=*%['] = function(code, res, char)
		char = char:sub(4, -2)
		local s, e = code:find(']' .. char .. ']', nil, true)
		if not s or not e then return code, res end

		-- Shift trailing spaces back
		local spaces = res:match('(%s*)$') or ''
		return spaces .. code:sub(e + 1), res:sub(1, #res - #spaces)
	end,

	-- Handle quoted text
	['"'] = function(code, res, char)
		res = res .. char

		-- Handle backslashes
		repeat
			local _, e, pattern = find_multiple(code, '\\', char)
			if pattern == char then
				res = res .. code:sub(1, e)
				code = code:sub(e + 1)
			elseif pattern then
				res = res .. code:sub(1, e + 1)
				code = code:sub(e + 2)
			end
		until not pattern or pattern == char

		return code, res
	end,

	['%s*[\r\n]%s*'] = function(code, res, char)
		return code, res .. '\n'
	end,

	['[ \t]+'] = function(code, res, char)
		return code, res .. ' '
	end,
}

-- Give the functions alternate names
matches["'"] = matches['"']

-- The actual transpiler
return function(code)
	assert(type(code) == 'string')

	local res = ''

	-- Split the code by "tokens"
	while true do
		-- Search for special characters
		local s, e, pattern = find_multiple(code, '[\'"\\]', '%-%-%[=*%[',
			'%-%-', '%[=*%[', '%s*[\r\n]%s*', '[ \t]+')
		if not s then break end

		-- Add non-matching characters
		res = res .. code:sub(1, math.max(s - 1, 0))

		-- Call the correct function
		local char = code:sub(s, e)
		local func = matches[char] or matches[pattern]
		assert(func, 'No function found for pattern!')
		code, res = func(code:sub(e + 1), res, char)
	end

	return (res .. code):trim()
end
