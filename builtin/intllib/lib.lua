local INS_CHAR = "@"
intllib.INSERTION_CHAR = INS_CHAR

local escapes = {
	["\\"] = "\\",
	["n"]  = "\n",
	["s"]  = " ",
	["t"]  = "\t",
	["r"]  = "\r",
	["f"]  = "\f",
	[INS_CHAR] = INS_CHAR..INS_CHAR,
}

local function unescape(str)
	local parts = {}
	local n = 1
	local function add(s)
		parts[n] = s
		n = n + 1
	end

	local start = 1
	while true do
		local pos = str:find("[\\@]", start)
		if pos then
			add(str:sub(start, pos - 1))
		else
			add(str:sub(start))
			break
		end
		local c = str:sub(pos + 1, pos + 1)
		if escapes[c] then
			add(escapes[c])
		elseif str:sub(pos, pos) == "@" then
			add("@" .. c)
		else
			add(c)
		end
		start = pos + 2
	end
	return table.concat(parts)
end

local function find_eq(s)
	for slashes, pos in s:gmatch("([\\]*)=()") do
		if (slashes:len() % 2) == 0 then
			return pos - 1
		end
	end
end

function intllib.load_strings(filename)
	local file, err = io.open(filename, "r")
	if not file then
		return nil, err
	end
	local strings = {}
	for line in file:lines() do
		line = line:trim()
		if line ~= "" and line:sub(1, 1) ~= "#" then
			local pos = find_eq(line)
			if pos then
				local msgid = unescape(line:sub(1, pos - 1):trim())
				strings[msgid] = unescape(line:sub(pos + 1):trim())
			end
		end
	end
	file:close()
	return strings
end
