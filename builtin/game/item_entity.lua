-- Minetest: builtin/item_entity.lua

local abs, min, floor, random, pi = math.abs, math.min, math.floor, math.random, math.pi
local vadd, vnormalize = vector.add, vector.normalize

function core.spawn_item(pos, item)
	-- Take item in any format
	local stack = ItemStack(item)
	local obj = core.add_entity(pos, "__builtin:item")
	-- Don't use obj if it couldn't be added to the map.
	if obj then
		obj:get_luaentity():set_item(stack:to_string())
	end
	return obj
end

-- If item_entity_ttl is not set, enity will have default life time
-- Setting it to -1 disables the feature

local time_to_live = tonumber(core.settings:get("item_entity_ttl")) or 600
local gravity = tonumber(core.settings:get("movement_gravity")) or 9.81
local collection = core.settings:get_bool("item_collection", true)
local water_flow = core.settings:get_bool("item_water_flow", true)

-- Water flow functions, based on QwertyMine3 (WTFPL), and TenPlus1 (MIT) mods
local function quick_flow_logic(node, pos_testing, dir)
	local node_testing = core.get_node_or_nil(pos_testing)
	if not node_testing then return 0 end

	local node_testing_def = core.registered_nodes[node_testing.name]
	local liquid = node_testing_def and node_testing_def.liquidtype

	if not liquid or (liquid ~= "flowing" and liquid ~= "source") then
		return 0
	end

	local sum = node.param2 - node_testing.param2

	return (sum < -6 or (sum < 6 and sum > 0) or sum == 0) and dir or -dir
end

local function quick_flow(pos, node)
	local x, z = 0, 0

	x = x + quick_flow_logic(node, {x = pos.x - 1.01, y = pos.y, z = pos.z}, -1)
	x = x + quick_flow_logic(node, {x = pos.x + 1.01, y = pos.y, z = pos.z},  1)
	z = z + quick_flow_logic(node, {x = pos.x, y = pos.y, z = pos.z - 1.01}, -1)
	z = z + quick_flow_logic(node, {x = pos.x, y = pos.y, z = pos.z + 1.01},  1)
	return vnormalize({x = x, y = 0, z = z})
end

core.register_entity(":__builtin:throwing_item", {
	physical = false,
	visual = "wielditem",
	collisionbox = {0, 0, 0, 0, 0, 0},
	textures = {""},
	visual_size = {x = 0.4, y = 0.4},
	is_visible = false,
	on_activate = function(self, staticdata)
		if staticdata == "expired" then
			self.object:remove()
		end
	end,
	get_staticdata = function()
		return "expired"
	end
})

core.register_entity(":__builtin:item", {
	initial_properties = {
		hp_max = 1,
		physical = true,
		collide_with_objects = false,
		collisionbox = {-0.3, -0.3, -0.3, 0.3, 0.3, 0.3},
		visual = "wielditem",
		visual_size = {x = 0.4, y = 0.4},
		textures = {""},
		is_visible = false,
	},

	itemstring = "",
	moving_state = true,
	physical_state = true,
	-- Item expiry
	age = 0,
	-- Pushing item out of solid nodes
	force_out = nil,
	force_out_start = nil,

	set_item = function(self, item)
		local stack = ItemStack(item or self.itemstring)
		self.itemstring = stack:to_string()
		if self.itemstring == "" then
			-- item not yet known
			return
		end

		-- Backwards compatibility: old clients use the texture
		-- to get the type of the item
		local itemname = stack:is_known() and stack:get_name() or "unknown"

		local max_count = stack:get_stack_max()
		local count = min(stack:get_count(), max_count)
		local size = 0.2 + 0.1 * (count / max_count) ^ (1 / 3)
		local def = core.registered_items[itemname]
		local glow = def and def.light_source and
			floor(def.light_source / 2 + 0.5)

		self.object:set_properties({
			is_visible = true,
			visual = "wielditem",
			textures = {itemname},
			visual_size = {x = size, y = size},
			collisionbox = {-size, -size, -size, size, size, size},
			automatic_rotate = pi * 0.5 * 0.15 / size,
			wield_item = self.itemstring,
			glow = glow,
			infotext = core.registered_items[itemname].description
		})

	end,

	get_staticdata = function(self)
		return core.serialize({
			itemstring = self.itemstring,
			age = self.age,
			dropped_by = self.dropped_by
		})
	end,

	on_activate = function(self, staticdata, dtime_s)
		if staticdata:sub(1, 6) == "return" then
			local data = core.deserialize(staticdata)
			if data and type(data) == "table" then
				self.itemstring = data.itemstring
				self.age = (data.age or 0) + dtime_s
				self.dropped_by = data.dropped_by
			end
		else
			self.itemstring = staticdata
		end
		self.object:set_armor_groups({immortal = 1, silent = 1})
		self.object:set_velocity({x = 0, y = 2, z = 0})
		self.object:set_acceleration({x = 0, y = -gravity, z = 0})
		self:set_item()
	end,

	try_merge_with = function(self, own_stack, object, entity)
		if self.age == entity.age then
			-- Can not merge with itself
			return false
		end

		local stack = ItemStack(entity.itemstring)
		local name = stack:get_name()
		if own_stack:get_name() ~= name or
				own_stack:get_meta() ~= stack:get_meta() or
				own_stack:get_wear() ~= stack:get_wear() or
				own_stack:get_free_space() == 0 then
			-- Can not merge different or full stack
			return false
		end

		local count = own_stack:get_count()
		local total_count = stack:get_count() + count
		local max_count = stack:get_stack_max()

		if total_count > max_count then
			return false
		end
		-- Merge the remote stack into this one

		local pos = object:get_pos()
		pos.y = pos.y + ((total_count - count) / max_count) * 0.15
		self.object:move_to(pos)

		self.age = 0 -- Handle as new entity
		own_stack:set_count(total_count)
		self:set_item(own_stack)

		entity.itemstring = ""
		object:remove()
		return true
	end,

	enable_physics = function(self)
		if not self.physical_state then
			self.physical_state = true
			self.object:set_properties({physical = true})
			self.object:set_velocity({x=0, y=0, z=0})
			self.object:set_acceleration({x=0, y=-gravity, z=0})
		end
	end,

	disable_physics = function(self)
		if self.physical_state then
			self.physical_state = false
			self.object:set_properties({physical = false})
			self.object:set_velocity({x=0, y=0, z=0})
			self.object:set_acceleration({x=0, y=0, z=0})
		end
	end,

	on_step = function(self, dtime, moveresult)
		self.age = self.age + dtime
		if time_to_live > 0 and self.age > time_to_live then
			self.itemstring = ""
			self.object:remove()
			return
		end

		local pos = self.object:get_pos()
		local node = core.get_node_or_nil({
			x = pos.x,
			y = pos.y + self.object:get_properties().collisionbox[2] - 0.05,
			z = pos.z
		})
		-- Delete in 'ignore' nodes
		if node and node.name == "ignore" then
			self.itemstring = ""
			self.object:remove()
			return
		end

		if self.force_out then
			-- This code runs after the entity got a push from the is_stuck code.
			-- It makes sure the entity is entirely outside the solid node
			local c = self.object:get_properties().collisionbox
			local s = self.force_out_start
			local f = self.force_out
			local ok = (f.x > 0 and pos.x + c[1] > s.x + 0.5) or
				(f.y > 0 and pos.y + c[2] > s.y + 0.5) or
				(f.z > 0 and pos.z + c[3] > s.z + 0.5) or
				(f.x < 0 and pos.x + c[4] < s.x - 0.5) or
				(f.z < 0 and pos.z + c[6] < s.z - 0.5)
			if ok then
				-- Item was successfully forced out
				self.force_out = nil
				self:enable_physics()
				return
			end
		end

		if not self.physical_state then
			return -- Don't do anything
		end

		assert(moveresult,
			"Collision info missing, this is caused by an out-of-date/buggy mod or game")

		if not moveresult.collides then
			-- future TODO: items should probably decelerate in air
			return
		end

		-- Push item out when stuck inside solid node
		local is_stuck = false
		local snode = core.get_node_or_nil(pos)
		if snode then
			local sdef = core.registered_nodes[snode.name] or {}
			is_stuck = (sdef.walkable == nil or sdef.walkable == true)
				and (sdef.collision_box == nil or sdef.collision_box.type == "regular")
				and (sdef.node_box == nil or sdef.node_box.type == "regular")
		end

		if is_stuck then
			local shootdir
			local order = {
				{x=1, y=0, z=0}, {x=-1, y=0, z= 0},
				{x=0, y=0, z=1}, {x= 0, y=0, z=-1},
			}

			-- Check which one of the 4 sides is free
			for o = 1, #order do
				local cnode = core.get_node(vadd(pos, order[o])).name
				local cdef = core.registered_nodes[cnode] or {}
				if cnode ~= "ignore" and cdef.walkable == false then
					shootdir = order[o]
					break
				end
			end
			-- If none of the 4 sides is free, check upwards
			if not shootdir then
				shootdir = {x=0, y=1, z=0}
				local cnode = core.get_node(vadd(pos, shootdir)).name
				if cnode == "ignore" then
					shootdir = nil -- Do not push into ignore
				end
			end

			if shootdir then
				-- Set new item moving speed accordingly
				local newv = vector.multiply(shootdir, 3)
				self:disable_physics()
				self.object:set_velocity(newv)

				self.force_out = newv
				self.force_out_start = vector.round(pos)
				return
			end
		end

		node = nil -- ground node we're colliding with
		if moveresult.touching_ground then
			for _, info in ipairs(moveresult.collisions) do
				if info.axis == "y" then
					node = core.get_node(info.node_pos)
					break
				end
			end
		end

		local vel = self.object:get_velocity()

		-- Moving items in the water flow (TenPlus1, MIT)
		local node_inside = core.get_node_or_nil(pos)
		local def_inside = node_inside and core.registered_nodes[node_inside.name]
		if water_flow and def_inside and def_inside.liquidtype == "flowing" then
			local vec = quick_flow(pos, node_inside)
			self.object:set_velocity({x = vec.x, y = vel.y, z = vec.z})
			return
		end

		-- Slide on slippery nodes
		local def = node and core.registered_nodes[node.name]
		local keep_movement = false

		if def then
			local slippery = core.get_item_group(node.name, "slippery")
			if slippery ~= 0 and (abs(vel.x) > 0.1 or abs(vel.z) > 0.1) then
				-- Horizontal deceleration
				local factor = min(4 / (slippery + 4) * dtime, 1)
				self.object:set_velocity({
					x = vel.x * (1 - factor),
					y = 0,
					z = vel.z * (1 - factor)
				})
				keep_movement = true
			end
		end

		if not keep_movement then
			self.object:set_velocity({x=0, y=0, z=0})
		end

		if self.moving_state == keep_movement then
			-- Do not update anything until the moving state changes
			return
		end
		self.moving_state = keep_movement

		-- Only collect items if not moving
		if self.moving_state then
			return
		end
		-- Collect the items around to merge with
		local own_stack = ItemStack(self.itemstring)
		if own_stack:get_free_space() == 0 then
			return
		end
		local objects = core.get_objects_inside_radius(pos, 0.5)
		for k, obj in pairs(objects) do
			local entity = obj:get_luaentity()
			if entity and entity.name == "__builtin:item" then
				if self:try_merge_with(own_stack, obj, entity) then
					own_stack = ItemStack(self.itemstring)
					if own_stack:get_free_space() == 0 then
						return
					end
				end
			end
		end
	end,

	on_punch = function(self, hitter)
		local inv = hitter:get_inventory()
		if inv and self.itemstring ~= "" then
			local left = inv:add_item("main", self.itemstring)
			if left and not left:is_empty() then
				self:set_item(left)
				return
			end
		end
		self.itemstring = ""
		self.object:remove()
	end,
})

-- Item Collection
if collection then
	local function collect_items(player)
		local ppos = player:get_pos()
		ppos.y = ppos.y + 1.3
		if not core.is_valid_pos(ppos) then
			return
		end

		-- Detect
		local objects = core.get_objects_inside_radius(ppos, 2)
		for _, obj in ipairs(objects) do
			local entity = obj:get_luaentity()
			if entity and entity.name == "__builtin:item" and
					not entity.collectioner and
					entity.age and entity.age > 0.5 then
				local item = ItemStack(entity.itemstring)
				local inv = player:get_inventory()
				if item:get_name() ~= "" and
						inv and inv:room_for_item("main", item) then
					-- Magnet
					obj:move_to(ppos)
					entity.collectioner = true

					-- Collect
					core.after(0.05, function()
						core.sound_play("item_drop_pickup", {
							pos = ppos,
							max_hear_distance = 10,
							gain = 0.2,
							pitch = random(60, 100) / 100
						}, true)
						entity.itemstring = ""
						obj:remove()
						item = inv:add_item("main", item)
						if not item:is_empty() then
							core.item_drop(item, player, ppos)
						end
					end)
				end
			end
		end
	end

	core.register_playerstep(function(_, playernames)
		for _, name in ipairs(playernames) do
			local player = core.get_player_by_name(name)
			if player and player:is_player() and player:get_hp() > 0 then
				collect_items(player)
			end
		end
	end, core.is_singleplayer()) -- Force step in singlplayer mode only
end
