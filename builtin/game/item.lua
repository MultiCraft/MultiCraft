-- Minetest: builtin/item.lua

local builtin_shared = ...

local abs, atan2, cos, floor, max, sin, random =
	math.abs, math.atan2, math.cos, math.floor, math.max, math.sin, math.random
local vadd, vnew, vmultiply, vnormalize, vsubtract =
	vector.add, vector.new, vector.multiply, vector.normalize, vector.subtract

local creative_mode = core.settings:get_bool("creative_mode")
local node_drop = core.settings:get_bool("node_drop")

local function copy_pointed_thing(pointed_thing)
	return {
		type  = pointed_thing.type,
		above = vnew(pointed_thing.above),
		under = vnew(pointed_thing.under),
		ref   = pointed_thing.ref,
	}
end

--
-- Item definition helpers
--

function core.inventorycube(img1, img2, img3)
	img2 = img2 or img1
	img3 = img3 or img1
	return "[inventorycube"
			.. "{" .. img1:gsub("%^", "&")
			.. "{" .. img2:gsub("%^", "&")
			.. "{" .. img3:gsub("%^", "&")
end

function core.get_pointed_thing_position(pointed_thing, above)
	if pointed_thing.type == "node" then
		if above then
			-- The position where a node would be placed
			return pointed_thing.above
		end
		-- The position where a node would be dug
		return pointed_thing.under
	elseif pointed_thing.type == "object" then
		return pointed_thing.ref and pointed_thing.ref:get_pos()
	end
end

function core.dir_to_facedir(dir, is6d)
	--account for y if requested
	if is6d and abs(dir.y) > abs(dir.x) and abs(dir.y) > abs(dir.z) then

		--from above
		if dir.y < 0 then
			if abs(dir.x) > abs(dir.z) then
				if dir.x < 0 then
					return 19
				else
					return 13
				end
			else
				if dir.z < 0 then
					return 10
				else
					return 4
				end
			end

		--from below
		else
			if abs(dir.x) > abs(dir.z) then
				if dir.x < 0 then
					return 15
				else
					return 17
				end
			else
				if dir.z < 0 then
					return 6
				else
					return 8
				end
			end
		end

	--otherwise, place horizontally
	elseif abs(dir.x) > abs(dir.z) then
		if dir.x < 0 then
			return 3
		else
			return 1
		end
	else
		if dir.z < 0 then
			return 2
		else
			return 0
		end
	end
end

-- Table of possible dirs
local facedir_to_dir = {
	{x= 0, y=0,  z= 1},
	{x= 1, y=0,  z= 0},
	{x= 0, y=0,  z=-1},
	{x=-1, y=0,  z= 0},
	{x= 0, y=-1, z= 0},
	{x= 0, y=1,  z= 0},
}
-- Mapping from facedir value to index in facedir_to_dir.
local facedir_to_dir_map = {
	[0]=1, 2, 3, 4,
	5, 2, 6, 4,
	6, 2, 5, 4,
	1, 5, 3, 6,
	1, 6, 3, 5,
	1, 4, 3, 2,
}
function core.facedir_to_dir(facedir)
	return facedir_to_dir[facedir_to_dir_map[facedir % 32]]
end

function core.dir_to_wallmounted(dir)
	if abs(dir.y) > max(abs(dir.x), abs(dir.z)) then
		if dir.y < 0 then
			return 1
		else
			return 0
		end
	elseif abs(dir.x) > abs(dir.z) then
		if dir.x < 0 then
			return 3
		else
			return 2
		end
	else
		if dir.z < 0 then
			return 5
		else
			return 4
		end
	end
end

-- table of dirs in wallmounted order
local wallmounted_to_dir = {
	[0] = {x = 0, y = 1, z = 0},
	{x =  0, y = -1, z =  0},
	{x =  1, y =  0, z =  0},
	{x = -1, y =  0, z =  0},
	{x =  0, y =  0, z =  1},
	{x =  0, y =  0, z = -1},
}
function core.wallmounted_to_dir(wallmounted)
	return wallmounted_to_dir[wallmounted % 8]
end

function core.dir_to_yaw(dir)
	return -atan2(dir.x, dir.z)
end

function core.yaw_to_dir(yaw)
	return {x = -sin(yaw), y = 0, z = cos(yaw)}
end

function core.is_colored_paramtype(ptype)
	return (ptype == "color") or (ptype == "colorfacedir") or
		(ptype == "colorwallmounted")
end

function core.strip_param2_color(param2, paramtype2)
	if not core.is_colored_paramtype(paramtype2) then
		return nil
	end
	if paramtype2 == "colorfacedir" then
		param2 = floor(param2 / 32) * 32
	elseif paramtype2 == "colorwallmounted" then
		param2 = floor(param2 / 8) * 8
	end
	-- paramtype2 == "color" requires no modification.
	return param2
end

function core.get_node_drops(node, toolname)
	-- Compatibility, if node is string
	local nodename = node
	local param2 = 0
	-- New format, if node is table
	if (type(node) == "table") then
		nodename = node.name
		param2 = node.param2
	end
	local def = core.registered_nodes[nodename]
	local drop = def and def.drop
	local ptype = def and def.paramtype2
	-- get color, if there is color (otherwise nil)
	local palette_index = core.strip_param2_color(param2, ptype)
	if drop == nil then
		-- default drop
		if palette_index then
			local stack = ItemStack(nodename)
			stack:get_meta():set_int("palette_index", palette_index)
			return {stack:to_string()}
		end
		return {nodename}
	elseif type(drop) == "string" then
		-- itemstring drop
		return drop ~= "" and {drop} or {}
	elseif drop.items == nil then
		-- drop = {} to disable default drop
		return {}
	end

	-- Extended drop table
	local got_items = {}
	local got_count = 0
	for _, item in ipairs(drop.items) do
		local good_rarity = true
		local good_tool = true
		if item.rarity ~= nil then
			good_rarity = item.rarity < 1 or random(item.rarity) == 1
		end
		if item.tools ~= nil then
			good_tool = false
		end
		if item.tools ~= nil and toolname then
			for _, tool in ipairs(item.tools) do
				if tool:sub(1, 1) == '~' then
					good_tool = toolname:find(tool:sub(2)) ~= nil
				else
					good_tool = toolname == tool
				end
				if good_tool then
					break
				end
			end
		end
		if good_rarity and good_tool then
			got_count = got_count + 1
			for _, add_item in ipairs(item.items) do
				-- add color, if necessary
				if item.inherit_color and palette_index then
					local stack = ItemStack(add_item)
					stack:get_meta():set_int("palette_index", palette_index)
					add_item = stack:to_string()
				end
				got_items[#got_items+1] = add_item
			end
			if drop.max_items ~= nil and got_count == drop.max_items then
				break
			end
		end
	end
	return got_items
end

local function user_name(user)
	return user and user:get_player_name() or ""
end

-- Returns a logging function. For empty names, does not log.
local function make_log(name)
	return name ~= "" and core.log or function() end
end

function core.item_place_node(itemstack, placer, pointed_thing, param2,
		prevent_after_place)
	local def = itemstack:get_definition()
	if def.type ~= "node" or pointed_thing.type ~= "node" then
		return itemstack, nil
	end

	local under = pointed_thing.under
	local oldnode_under = core.get_node_or_nil(under)
	local above = pointed_thing.above
	local oldnode_above = core.get_node_or_nil(above)
	local playername = user_name(placer)
	local log = make_log(playername)

	if not oldnode_under or not oldnode_above then
		log("info", playername .. " tried to place"
			.. " node in unloaded position " .. core.pos_to_string(above))
		return itemstack, nil
	end

	local olddef_under = core.registered_nodes[oldnode_under.name]
	olddef_under = olddef_under or core.nodedef_default
	local olddef_above = core.registered_nodes[oldnode_above.name]
	olddef_above = olddef_above or core.nodedef_default

	if not olddef_above.buildable_to and not olddef_under.buildable_to then
		log("info", playername .. " tried to place"
			.. " node in invalid position " .. core.pos_to_string(above)
			.. ", replacing " .. oldnode_above.name)
		return itemstack, nil
	end

	-- Place above pointed node
	local place_to = {x = above.x, y = above.y, z = above.z}

	-- If node under is buildable_to, place into it instead (eg. snow)
	if olddef_under.buildable_to then
		log("info", "node under is buildable to")
		place_to = {x = under.x, y = under.y, z = under.z}
	end

	if core.is_protected(place_to, playername) then
		log("action", playername
				.. " tried to place " .. def.name
				.. " at protected position "
				.. core.pos_to_string(place_to))
		core.record_protection_violation(place_to, playername)
		return itemstack, nil
	end

	local oldnode = core.get_node(place_to)
	local newnode = {name = def.name, param1 = 0, param2 = param2 or 0}

	-- Calculate direction for wall mounted stuff like torches and signs
	if def.place_param2 ~= nil then
		newnode.param2 = def.place_param2
	elseif (def.paramtype2 == "wallmounted" or
			def.paramtype2 == "colorwallmounted") and not param2 then
		local dir = {
			x = under.x - above.x,
			y = under.y - above.y,
			z = under.z - above.z
		}
		newnode.param2 = core.dir_to_wallmounted(dir)
	-- Calculate the direction for furnaces and chests and stuff
	elseif (def.paramtype2 == "facedir" or
			def.paramtype2 == "colorfacedir") and not param2 then
		local placer_pos = placer and placer:get_pos()
		if placer_pos then
			local dir = {
				x = above.x - placer_pos.x,
				y = above.y - placer_pos.y,
				z = above.z - placer_pos.z
			}
			newnode.param2 = core.dir_to_facedir(dir)
			log("info", "facedir: " .. newnode.param2)
		end
	end

	local metatable = itemstack:get_meta():to_table().fields

	-- Transfer color information
	if metatable.palette_index and not def.place_param2 then
		local color_divisor = nil
		if def.paramtype2 == "color" then
			color_divisor = 1
		elseif def.paramtype2 == "colorwallmounted" then
			color_divisor = 8
		elseif def.paramtype2 == "colorfacedir" then
			color_divisor = 32
		end
		if color_divisor then
			local color = floor(metatable.palette_index / color_divisor)
			local other = newnode.param2 % color_divisor
			newnode.param2 = color * color_divisor + other
		end
	end

	-- Check if the node is attached and if it can be placed there
	if core.get_item_group(def.name, "attached_node") ~= 0 and
		not builtin_shared.check_attached_node(place_to, newnode) then
		log("action", "attached node " .. def.name ..
			" can not be placed at " .. core.pos_to_string(place_to))
		return itemstack, nil
	end

	log("action", playername .. " places node "
		.. def.name .. " at " .. core.pos_to_string(place_to))

	-- Add node and update
	core.add_node(place_to, newnode)

	-- Play sound if it was done by a player
	if playername ~= "" and def.sounds and def.sounds.place then
		core.sound_play(def.sounds.place, {
			pos = place_to,
			exclude_player = playername,
		}, true)
	end

	local take_item = true

	-- Run callback
	if def.after_place_node and not prevent_after_place then
		-- Deepcopy place_to and pointed_thing because callback can modify it
		local place_to_copy = {x=place_to.x, y=place_to.y, z=place_to.z}
		local pointed_thing_copy = copy_pointed_thing(pointed_thing)
		if def.after_place_node(place_to_copy, placer, itemstack,
				pointed_thing_copy) then
			take_item = false
		end
	end

	-- Run script hook
	for _, callback in ipairs(core.registered_on_placenodes) do
		-- Deepcopy pos, node and pointed_thing because callback can modify them
		local place_to_copy = {x=place_to.x, y=place_to.y, z=place_to.z}
		local newnode_copy = {name=newnode.name, param1=newnode.param1, param2=newnode.param2}
		local oldnode_copy = {name=oldnode.name, param1=oldnode.param1, param2=oldnode.param2}
		local pointed_thing_copy = copy_pointed_thing(pointed_thing)
		if callback(place_to_copy, newnode_copy, placer, oldnode_copy, itemstack, pointed_thing_copy) then
			take_item = false
		end
	end

	if take_item then
		itemstack:take_item()
	end
	return itemstack, place_to
end

function core.item_place(itemstack, placer, pointed_thing, param2)
	-- Call on_rightclick if the pointed node defines it
	if pointed_thing.type == "node" and placer and
			not placer:get_player_control().sneak then
		local n = core.get_node(pointed_thing.under)
		local nn = n.name
		if core.registered_nodes[nn] and core.registered_nodes[nn].on_rightclick then
			return core.registered_nodes[nn].on_rightclick(pointed_thing.under, n,
					placer, itemstack, pointed_thing) or itemstack, nil
		end
	end

	-- Place if node, otherwise do nothing
	if itemstack:get_definition().type == "node" then
		return core.item_place_node(itemstack, placer, pointed_thing, param2)
	end
	return itemstack, nil
end

function core.item_secondary_use(itemstack, placer)
	return itemstack
end

local function item_throw_step(entity, dtime)
	entity.throw_timer = entity.throw_timer + dtime
	if entity.throw_timer > 20 then
		entity.object:remove()
		return
	end
	if not entity.thrower then
		return
	end
	local pos = entity.object:get_pos()
	if not core.is_valid_pos(pos) then
		entity.object:remove()
		return
	end
	local hit_object
	local dir = vnormalize(entity.object:get_velocity())
	local pos2 = vadd(pos, vmultiply(dir, 3))
	local _, node_pos = core.line_of_sight(pos, pos2)
	if node_pos then
		local def = core.get_node(node_pos)
		if def then
			pos = vsubtract(node_pos, vmultiply(dir, 1.5))
			entity.object:move_to(pos)
		else
			node_pos = nil
		end
	end
	local objs = core.get_objects_inside_radius(pos, 1.5)
	for _, obj in pairs(objs) do
		if obj:is_player() then
			local name = obj:get_player_name()
			if name ~= entity.thrower then
				hit_object = obj
				break
			end
		elseif obj:get_luaentity() ~= nil and
				obj:get_luaentity().name ~= "__builtin:throwing_item" and
				obj:get_luaentity().name ~= "__builtin:item" then
			hit_object = obj
			break
		end
	end
	if hit_object or node_pos then
		local player = core.get_player_by_name(entity.thrower)
		entity.on_impact(player, pos, entity.throw_direction, hit_object)
		entity.object:remove()
	end
end

function core.item_throw(name, thrower, speed, accel, on_impact)
	if not thrower or not thrower:is_player() then
		return
	end
	local pos = thrower:get_pos()
	if not core.is_valid_pos(pos) then
		return
	end
	pos.y = pos.y + 1.5
	local obj
	local properties = {is_visible=true}
	if core.registered_entities[name] then
		obj = core.add_entity(pos, name)
	elseif core.registered_items[name] then
		obj = core.add_entity(pos, "__builtin:throwing_item")
		properties.textures = {name}
	else
		return
	end
	if obj then
		local ent = obj:get_luaentity()
		if ent then
			local s = speed or 19 -- default speed
			local a = accel or -3 -- default acceleration
			local dir = thrower:get_look_dir()
			local gravity = tonumber(core.settings:get("movement_gravity")) or 9.81
			ent.thrower = thrower:get_player_name()
			ent.throw_timer = 0
			ent.throw_direction = dir
			ent.on_step = item_throw_step
			ent.on_impact = on_impact and on_impact or function() end
			obj:set_properties(properties)
			obj:set_velocity({x=dir.x * s, y=dir.y * s, z=dir.z * s})
			obj:set_acceleration({x=dir.x * a, y=-gravity, z=dir.z * a})
			return obj
		else
			obj:remove()
		end
	end
end

function core.item_drop(itemstack, dropper, pos)
	local dropper_is_player = dropper and dropper:is_player()
	local p = table.copy(pos)
	local cnt = itemstack:get_count()
	if not core.is_valid_pos(p) then
		return
	end
	if dropper_is_player then
		p.y = p.y + 1.2
	end
	local item = itemstack:take_item(cnt)
	local obj = core.add_item(p, item)
	if obj then
		if dropper_is_player then
			local vel = dropper.get_velocity and dropper:get_velocity() or vnew
			local dir = dropper:get_look_dir()
			dir.x = vel.x + dir.x * 4
			dir.y = vel.y + dir.y * 4 + 2
			dir.z = vel.z + dir.z * 4
			obj:set_velocity(dir)
			obj:get_luaentity().dropped_by = dropper:get_player_name()
		else
			obj:set_velocity({
				x = random(-2, 2),
				y = random(2, 4),
				z = random(-2, 2)
			})
		end
		return itemstack
	end
	-- If we reach this, adding the object to the
	-- environment failed
end

local enable_hunger = core.settings:get_bool("enable_damage") and core.settings:get_bool("enable_hunger")
function core.do_item_eat(hp_change, replace_with_item, itemstack, user, pointed_thing, poison)
	for _, callback in pairs(core.registered_on_item_eats) do
		local result = callback(hp_change, replace_with_item, itemstack, user, pointed_thing)
		if result then
			return result
		end
	end
	local def = itemstack:get_definition()
	if itemstack:take_item() ~= nil then
		if enable_hunger then
			hunger.item_eat(hp_change, user, poison)
		else
			user:set_hp(user:get_hp() + hp_change)
		end

		local pos = user:get_pos()
		if not core.is_valid_pos(pos) then
			return itemstack
		end

		if def and def.sound and def.sound.eat then
			core.sound_play(def.sound.eat, {
				pos = pos,
				max_hear_distance = 16
			}, true)
		else
			core.sound_play("player_eat", {
				pos = pos,
				max_hear_distance = 16,
				gain = 0.3
			}, true)
		end

		local dir = user:get_look_dir()
		local ppos = {x = pos.x, y = pos.y + 1.3, z = pos.z}
		core.add_particlespawner({
			amount = 20,
			time = 0.1,
			minpos = ppos,
			maxpos = ppos,
			minvel = {x = dir.x - 1, y = 2, z = dir.z - 1},
			maxvel = {x = dir.x + 1, y = 2, z = dir.z + 1},
			minacc = {x = 0, y = -5, z = 0},
			maxacc = {x = 0, y = -9, z = 0},
			minexptime = 1,
			maxexptime = 1,
			minsize = 1,
			maxsize = 1,
			vertical = false,
			texture = def.inventory_image
		})

		if replace_with_item then
			if itemstack:is_empty() then
				itemstack:add_item(replace_with_item)
			else
				local inv = user:get_inventory()
				-- Check if inv is null, since non-players don't have one
				if inv and inv:room_for_item("main", {name=replace_with_item}) then
					inv:add_item("main", replace_with_item)
				else
					pos.y = pos.y + 0.5
					core.add_item(pos, replace_with_item)
				end
			end
		end
	end
	return itemstack
end

function core.item_eat(hp_change, replace_with_item, poison)
	return function(itemstack, user, pointed_thing)  -- closure
		if user then
			if user:is_player() and pointed_thing.type == "object" then
				pointed_thing.ref:right_click(user)
				return user:get_wielded_item()
			end

			return core.do_item_eat(hp_change, replace_with_item, itemstack, user, pointed_thing, poison)
		end
	end
end

function core.node_punch(pos, node, puncher, pointed_thing)
	-- Run script hook
	for _, callback in ipairs(core.registered_on_punchnodes) do
		-- Copy pos and node because callback can modify them
		local pos_copy = vnew(pos)
		local node_copy = {name=node.name, param1=node.param1, param2=node.param2}
		local pointed_thing_copy = pointed_thing and copy_pointed_thing(pointed_thing) or nil
		callback(pos_copy, node_copy, puncher, pointed_thing_copy)
	end
end

function core.handle_node_drops(pos, drops, digger)
	-- Add dropped items to object's inventory
	local inv = digger and digger:get_inventory()
	local give_item
	if (not node_drop or creative_mode) and inv then
		give_item = function(item)
			return inv:add_item("main", item)
		end
	else
		give_item = function(item)
			-- itemstring to ItemStack for left:is_empty()
			return ItemStack(item)
		end
	end

	for _, dropped_item in pairs(drops) do
		local left = give_item(dropped_item)
		if not left:is_empty() then
			core.item_drop(left, nil, pos)
		end
	end
end

function core.node_dig(pos, node, digger)
	local diggername = user_name(digger)
	local log = make_log(diggername)
	local def = core.registered_nodes[node.name]
	-- Copy pos because the callback could modify it
	if def and (not def.diggable or
			(def.can_dig and not def.can_dig(vector.new(pos), digger))) then
		log("info", diggername .. " tried to dig "
			.. node.name .. " which is not diggable "
			.. core.pos_to_string(pos))
		return false
	end

	if core.is_protected(pos, diggername) then
		log("action", diggername
				.. " tried to dig " .. node.name
				.. " at protected position "
				.. core.pos_to_string(pos))
		core.record_protection_violation(pos, diggername)
		return false
	end

	log('action', diggername .. " digs "
		.. node.name .. " at " .. core.pos_to_string(pos))

	local wielded = digger and digger:get_wielded_item()
	local drops = core.get_node_drops(node, wielded and wielded:get_name())

	if wielded then
		local wdef = wielded:get_definition()
		local tp = wielded:get_tool_capabilities()
		local dp = core.get_dig_params(def and def.groups, tp)
		if wdef and wdef.after_use then
			wielded = wdef.after_use(wielded, digger, node, dp) or wielded
		else
			-- Wear out tool
			if not core.is_creative_enabled(diggername) then
				wielded:add_wear(dp.wear)
				if wielded:get_count() == 0 and wdef.sound and wdef.sound.breaks then
					core.sound_play(wdef.sound.breaks, {
						pos = pos,
						gain = 0.5
					}, true)
				end
			end
		end
		digger:set_wielded_item(wielded)
	end

	-- Check to see if metadata should be preserved.
	if def and def.preserve_metadata then
		local oldmeta = core.get_meta(pos):to_table().fields
		-- Copy pos and node because the callback can modify them.
		local pos_copy = {x=pos.x, y=pos.y, z=pos.z}
		local node_copy = {name=node.name, param1=node.param1, param2=node.param2}
		local drop_stacks = {}
		for k, v in pairs(drops) do
			drop_stacks[k] = ItemStack(v)
		end
		drops = drop_stacks
		def.preserve_metadata(pos_copy, node_copy, oldmeta, drops)
	end

	-- Handle drops
	core.handle_node_drops(pos, drops, digger)

	local oldmetadata = nil
	if def and def.after_dig_node then
		oldmetadata = core.get_meta(pos):to_table()
	end

	-- Remove node and update
	core.remove_node(pos)

	-- Play sound if it was done by a player
	if diggername ~= "" and def and def.sounds and def.sounds.dug then
		core.sound_play(def.sounds.dug, {
			pos = pos,
			exclude_player = diggername,
		}, true)
	end

	-- Run callback
	if def and def.after_dig_node then
		-- Copy pos and node because callback can modify them
		local pos_copy = {x=pos.x, y=pos.y, z=pos.z}
		local node_copy = {name=node.name, param1=node.param1, param2=node.param2}
		def.after_dig_node(pos_copy, node_copy, oldmetadata, digger)
	end

	-- Run script hook
	for _, callback in ipairs(core.registered_on_dignodes) do
		local origin = core.callback_origins[callback]
		if origin then
			core.set_last_run_mod(origin.mod)
		end

		-- Copy pos and node because callback can modify them
		local pos_copy = {x=pos.x, y=pos.y, z=pos.z}
		local node_copy = {name=node.name, param1=node.param1, param2=node.param2}
		callback(pos_copy, node_copy, digger)
	end

	return true
end

function core.itemstring_with_palette(item, palette_index)
	local stack = ItemStack(item) -- convert to ItemStack
	stack:get_meta():set_int("palette_index", palette_index)
	return stack:to_string()
end

function core.itemstring_with_color(item, colorstring)
	local stack = ItemStack(item) -- convert to ItemStack
	stack:get_meta():set_string("color", colorstring)
	return stack:to_string()
end

-- This is used to allow mods to redefine core.item_place and so on
-- NOTE: This is not the preferred way. Preferred way is to provide enough
--       callbacks to not require redefining global functions. -celeron55
local function redef_wrapper(table, name)
	return function(...)
		return table[name](...)
	end
end

--
-- Item definition defaults
--

local default_stack_max = tonumber(core.settings:get("default_stack_max")) or 64

core.nodedef_default = {
	-- Item properties
	type="node",
	-- name intentionally not defined here
	description = "",
	groups = {},
	inventory_image = "",
	wield_image = "",
	wield_scale = {x=1,y=1,z=1},
	stack_max = default_stack_max,
	usable = false,
	liquids_pointable = false,
	tool_capabilities = nil,
	node_placement_prediction = nil,

	-- Interaction callbacks
	on_place = redef_wrapper(core, 'item_place'), -- core.item_place
	on_drop = redef_wrapper(core, 'item_drop'), -- core.item_drop
	on_use = nil,
	can_dig = nil,

	on_punch = redef_wrapper(core, 'node_punch'), -- core.node_punch
	on_rightclick = nil,
	on_dig = redef_wrapper(core, 'node_dig'), -- core.node_dig

	on_receive_fields = nil,

	-- Node properties
	drawtype = "normal",
	visual_scale = 1.0,
	-- Don't define these because otherwise the old tile_images and
	-- special_materials wouldn't be read
	--tiles ={""},
	--special_tiles = {
	--	{name="", backface_culling=true},
	--	{name="", backface_culling=true},
	--},
	post_effect_color = {a=0, r=0, g=0, b=0},
	paramtype = "none",
	paramtype2 = "none",
	is_ground_content = true,
	sunlight_propagates = false,
	walkable = true,
	pointable = true,
	diggable = true,
	climbable = false,
	buildable_to = false,
	floodable = false,
	liquidtype = "none",
	liquid_alternative_flowing = "",
	liquid_alternative_source = "",
	liquid_viscosity = 0,
	drowning = 0,
	light_source = 0,
	damage_per_second = 0,
	selection_box = {type="regular"},
	legacy_facedir_simple = false,
	legacy_wallmounted = false,
}

core.craftitemdef_default = {
	type="craft",
	-- name intentionally not defined here
	description = "",
	groups = {},
	inventory_image = "",
	wield_image = "",
	wield_scale = {x=1,y=1,z=1},
	stack_max = default_stack_max,
	liquids_pointable = false,
	tool_capabilities = nil,

	-- Interaction callbacks
	on_place = redef_wrapper(core, 'item_place'), -- core.item_place
	on_drop = redef_wrapper(core, 'item_drop'), -- core.item_drop
	on_secondary_use = redef_wrapper(core, 'item_secondary_use'),
	on_use = nil,
}

core.tooldef_default = {
	type="tool",
	-- name intentionally not defined here
	description = "",
	groups = {},
	inventory_image = "",
	wield_image = "",
	wield_scale = {x=1,y=1,z=1},
	stack_max = 1,
	liquids_pointable = false,
	tool_capabilities = nil,

	-- Interaction callbacks
	on_place = redef_wrapper(core, 'item_place'), -- core.item_place
	on_secondary_use = redef_wrapper(core, 'item_secondary_use'),
	on_drop = redef_wrapper(core, 'item_drop'), -- core.item_drop
	on_use = nil,
}

core.noneitemdef_default = {  -- This is used for the hand and unknown items
	type="none",
	-- name intentionally not defined here
	description = "",
	groups = {},
	inventory_image = "",
	wield_image = "",
	wield_scale = {x=1,y=1,z=1},
	stack_max = default_stack_max,
	liquids_pointable = false,
	tool_capabilities = nil,

	-- Interaction callbacks
	on_place = redef_wrapper(core, 'item_place'),
	on_secondary_use = redef_wrapper(core, 'item_secondary_use'),
	on_drop = nil,
	on_use = nil,
}
