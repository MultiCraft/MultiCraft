--
-- A primitive-ish code minifier
--
-- Copyright © 2023 by luk3yx
-- Copyright © 2023 MultiCraft Development Team
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

local find, sub, byte = string.find, string.sub, string.byte
local QUOTE, APOSTROPHE, LBRACKET, BACKSLASH, SPACE, TAB, CR, NEWLINE, HYPHEN,
		DOT, EQUALS, COLON = byte('"\'[\\ \t\r\n-.=:', 1, -1)

local function set(...)
	local res = {}
	for _, item in ipairs({...}) do
		res[item] = true
	end
	return res
end

local whitespace_bytes = set(SPACE, TAB, CR, NEWLINE)

local function parse_long_string(code, idx, lineno)
	local marker_start, marker_end = find(code, "^%[=*%[", idx)
	if not marker_start then
		return nil, lineno
	end

	local end_symbol = "]" .. ("="):rep(marker_end - marker_start - 1) .. "]"
	local _, end_idx = find(code, end_symbol, marker_end + 1, true)
	end_idx = end_idx or #code + 1

	for i = marker_end + 1, end_idx - 1 do
		if byte(code, i) == NEWLINE then
			lineno = lineno + 1
		end
	end

	return end_idx, lineno
end

local function tokenise(code)
	local last_idx = 1
	local lineno = 1
	local next_token
	return function()
		-- Return the next token if set
		if next_token ~= nil then
			local res = next_token
			next_token = nil
			return res, lineno
		end

		local start_idx, end_idx = find(code, "[%(%):%.\n\"' \t\r%+%-%*/~=<>|;%[%],%{%}#%%%^]", last_idx)
		if not start_idx then
			-- HACK: Get the last token
			if byte(code, -1) == NEWLINE then return end
			code = code .. "\n"
			start_idx, end_idx = #code, #code
		end

		local symbol = byte(code, start_idx)

		if symbol == QUOTE or symbol == APOSTROPHE then
			-- Quoted strings
			local new_symbol
			repeat
				end_idx = end_idx + 1
				new_symbol = byte(code, end_idx)
				-- Skip over backslashes
				if new_symbol == BACKSLASH then
					end_idx = end_idx + 1
				end
			until symbol == new_symbol or new_symbol == nil
		elseif whitespace_bytes[symbol] then
			-- Consume spaces/tabs
			while true do
				if symbol == NEWLINE then lineno = lineno + 1 end

				symbol = byte(code, end_idx + 1)
				if not whitespace_bytes[symbol] then break end
				end_idx = end_idx + 1
			end
		elseif symbol == LBRACKET then
			-- Long strings (potentially)
			end_idx, lineno = parse_long_string(code, start_idx, lineno)
			if not end_idx then end_idx = start_idx end
		elseif symbol == HYPHEN and byte(code, start_idx + 1) == HYPHEN then
			-- Comments
			end_idx, lineno = parse_long_string(code, start_idx + 2, lineno)
			if not end_idx then
				end_idx = find(code, "\n", start_idx, true) or #code + 1
				lineno = lineno + 1
			end
		elseif symbol == DOT and byte(code, start_idx + 1) == DOT then
			-- Varargs and string concatenation
			end_idx = byte(code, start_idx + 2) == DOT and start_idx + 2 or start_idx + 1
		elseif (symbol == COLON or symbol == EQUALS) and byte(code, start_idx + 1) == symbol then
			-- == and goto labels
			end_idx = start_idx + 1
		end
		next_token = sub(code, start_idx, end_idx)

		local res = sub(code, last_idx, start_idx - 1)
		if res == "" and next_token then
			res, next_token = next_token, nil
		end
		last_idx = end_idx + 1
		return res, lineno
	end
end

-- local string_starting_tokens = set(QUOTE, APOSTROPHE, LBRACKET)

local keywords = set("and", "break", "do", "else", "elseif", "end", "for",
	"function", "if", "in", "local", "not", "or", "repeat", "return", "then",
	"until", "while")
local scope_begin = set("then", "do", "repeat")
local scope_end = set("end", "until", "elseif")
local operators = set("+", "-", "*", "/", "^", "%", "=", "==", "~=", "<", "<<",
	"<=", ">", ">>", ">=", "&", "|", "~", "//", "and", "or", "..", "#", "not")

local function copy_scope(scope)
	return setmetatable({}, {__index = scope})
end

local function minify(code)
	local locals_count = 0
	local function new_var_name(scope, name)
		locals_count = locals_count + 1
		local var = ("_%x"):format(locals_count)
		scope[name] = var
		return var
	end


	-- Remove whitespace and comments when iterating
	local raw_token_iterator = tokenise(code)
	local need_whitespace = false
	local next_token
	local lineno_cache = -1
	local function token_iterator()
		if next_token ~= nil then
			local token = next_token
			next_token = nil
			return token, lineno_cache
		end

		for token, lineno in raw_token_iterator do
			local first_byte = byte(token, 1)
			if whitespace_bytes[first_byte] or
					(first_byte == HYPHEN and byte(token, 2) == HYPHEN) then
				need_whitespace = true
			else
				lineno_cache = lineno
				return token, lineno
			end
		end
	end

	-- Write tokens and any whitespace if necessary
	local res = {}
	local last_token = ""
	local function write_token(token)
		if need_whitespace then
			need_whitespace = false
			if (token:find("^[A-Za-z0-9_]") and last_token:find("[A-Za-z0-9_]$")) or
					(token == ".." and last_token:find("[0-9]$") or
					(token == "-" and last_token == "-")) then
				res[#res + 1] = " "
			end
		end

		res[#res + 1] = token
		last_token = token
	end

	-- Expects to be called after the ( token
	local process_block
	local function process_function(old_scope)
		local scope = copy_scope(old_scope)
		write_token("(")
		local token, lineno = token_iterator()
		while token do
			if token == ")" then
				write_token(")")
				process_block(copy_scope(scope), "function")
				return
			end
			assert(not scope_begin[token] and not scope_end[token], token)

			if token ~= "..." then
				token = new_var_name(scope, token)
			end
			write_token(token)

			token, lineno = token_iterator()
			if token == "," then
				write_token(",")
				token, lineno = token_iterator()
			elseif token ~= ")" then
				error(("Unexpected token %q on line %d"):format(token, lineno))
			end
		end

		error("Unexpected EOF on line " .. lineno)
	end

	local function process_declaration(scope)
		write_token("local")

		local token = token_iterator()
		if token == "function" then
			write_token("function")
			write_token(new_var_name(scope, token_iterator()))
			assert(token_iterator() == "(", "Expected ( after local function")
			process_function(scope)
			return
		end

		local var_names = {}
		local var_count = 0
		while true do
			-- Variable name
			write_token(token)
			var_names[#var_names + 1] = #res
			var_count = var_count + 1

			token = token_iterator()
			if token == "," then
				write_token(token)
				token = token_iterator()
			elseif token == "=" then
				write_token("=")
				for i = 1, var_count do
					process_block(scope, "local")
					if i < var_count then
						token = token_iterator()
						if token ~= "," then
							next_token = token
							break
						end
						write_token(",")
					end
				end
				break
			else
				next_token = token
				break
			end
		end

		for _, res_idx in ipairs(var_names) do
			res[res_idx] = new_var_name(scope, res[res_idx])
		end
	end

	local function process_table(scope)
		write_token("{")
		while true do
			local token = token_iterator()

			-- Key
			local expect_value = false
			if token ~= "function" and token:find("^[A-Za-z_][A-Za-z0-9_]*$") then
				assert(not keywords[token])

				-- If this is a variable name then swap it with the scope
				next_token = token_iterator()
				expect_value = next_token == "="
				if expect_value then
					write_token(token)
				else
					write_token(scope[token] or token)
					if next_token ~= "," and next_token ~= "}" then
						need_whitespace = true
						process_block(scope, "{")
					end
				end
			elseif token == "[" then
				write_token("[")
				process_block(scope, "[")
				expect_value = true
			elseif token == "}" then
				if res[#res] == "," then
					res[#res] = "}"
				else
					write_token(token)
				end
				break
			else
				next_token = token
				process_block(scope, "{")
			end

			-- Separator (, or =)
			local lineno
			token, lineno = token_iterator()
			if expect_value then
				assert(token == "=")
				write_token("=")
				process_block(scope, "{")

				token, lineno = token_iterator()
			end

			write_token(token)

			if token == "}" then
				break
			elseif token ~= "," then
				error(("Unexpected token %q on line %d, expected ','"):format(token, lineno))
			end
		end
	end

	function process_block(scope, block_start)
		local for_scope
		for token, lineno in token_iterator do
			local token_first_byte = byte(token)
			if whitespace_bytes[token_first_byte] then
				if last_token and last_token:find("[A-Za-z0-9_]$") then
					need_whitespace = true
				end
			elseif token == "function" then
				-- Process functions
				-- TODO: Maybe make this verify syntax?
				write_token("function")
				local first = true
				for token2 in token_iterator do
					if token2 == "(" then
						process_function(scope)
						break
					end
					write_token(first and scope[token2] or token2)
					first = false
				end
			elseif token == "local" then
				-- Process local declarations
				assert(block_start ~= "local")
				process_declaration(scope)
			elseif token == "for" then
				assert(block_start ~= "local")
				write_token("for")
				for_scope = copy_scope(scope)
				for_scope["?"] = lineno
				for token2 in token_iterator do
					if token2 == "," then
						write_token(token2)
					elseif token2 == "in" or token2 == "=" then
						write_token(token2)
						break
					else
						write_token(new_var_name(for_scope, token2))
					end
				end
			elseif scope_begin[token] then
				write_token(token)
				if not for_scope then
					process_block(copy_scope(scope), token)
				elseif token ~= "do" then
					error(("Unexpected token %q on line %d (for statement on line %d)"):format(token, lineno, for_scope["?"]))
				else
					process_block(for_scope, token)
					for_scope = nil
				end
			elseif scope_end[token] then
				if token == "elseif" then
					assert(block_start == "then", "Mismatched elseif")
				elseif block_start == "repeat" then
					assert(token == "until", "Mismatched until")
					write_token(token)
					process_block(scope, "local")
					return
				elseif token ~= "end" or block_start == nil or block_start == "(" or block_start == "local" then
					error("Mismatched end on line " .. lineno)
				end

				write_token(token)
				return

			-- Use recursion with brackets if required
			elseif (token == "(" or token == "[") and
					(token == block_start or block_start == "{" or block_start == "local") then
				write_token(token)
				process_block(scope, token)
			elseif (token == ")" and block_start == "(") or
					(token == "]" and block_start == "[") then
				write_token(token)
				return

			elseif token == "else" then
				assert(block_start == "then", "Mismatched else")

				-- Reset the scope
				for var in pairs(scope) do
					scope[var] = nil
				end
				write_token(token)

			elseif token == "{" then
				-- Parse tables
				process_table(scope)

			elseif token == "." or token == ":" then
				-- Parse attributes
				write_token(token)

				local new_token = assert(token_iterator(), "Expected attribute, got EOF")
				if new_token:find("^[A-Za-z0-9_]") then
					-- Attribute (or decimal place on a number)
					write_token(new_token)
				else
					-- Not an attribute (for example 1.)
					next_token = new_token
				end
			else
				write_token(scope[token] or token)
			end

			-- Peek at the next token
			if (block_start == "local" or block_start == "{") and not operators[token] then
				next_token = token_iterator()
				if next_token == nil or next_token == "," or
						(block_start == "{" and next_token == "}") or (next_token ~= "and" and
						next_token ~= "or" and next_token:find("^[A-Za-z0-9_]")) then
					return
				end
			end
		end
	end

	process_block({})
	local token, lineno = token_iterator()
	if token then
		error(("Unexpected data %q on line %d, expected EOF"):format(token, lineno))
	end

	return table.concat(res)
end

return minify
