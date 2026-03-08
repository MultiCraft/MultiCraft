-- can be overriden by mods
local exp = math.exp
function core.calculate_knockback(player, hitter, time_from_last_punch, tool_capabilities, dir, distance, damage)
	if damage == 0 or player:get_armor_groups().immortal then
		return 0.0
	end

	local m = 8
	-- solve m - m*e^(k*4) = 4 for k
	local k = -0.17328
	local res = m - m * exp(k * damage)

	if distance < 2.0 then
		res = res * 1.1 -- more knockback when closer
	elseif distance > 4.0 then
		res = res * 0.9 -- less when far away
	end
	return res
end

local max, abs = math.max, math.abs
local function vector_absmax(v)
	return max(max(abs(v.x), abs(v.y)), abs(v.z))
end

local vadd, vdivide, vlength, vsubtract = vector.add, vector.divide, vector.length, vector.subtract
core.register_on_punchplayer(function(player, hitter, time_from_last_punch, tool_capabilities, unused_dir, damage)
	if player:get_hp() == 0 then
		return -- RIP
	end

	-- Server::handleCommand_Interact() adds eye offset to one but not the other
	-- so the direction is slightly off, calculate it ourselves
	local player_pos = player:get_pos()
	local dir = vsubtract(player_pos, hitter:get_pos())
	local d = vlength(dir)
	if d ~= 0.0 then
		dir = vdivide(dir, d)
	end

	local k = core.calculate_knockback(player, hitter, time_from_last_punch, tool_capabilities, dir, d, damage)

	local kdir = vector.multiply(dir, k)
	if vector_absmax(kdir) < 1.0 then
		return -- barely noticeable, so don't even send
	end

	if core.is_valid_pos(vadd(player_pos, kdir)) then
		player:add_velocity(kdir)
	end
end)
