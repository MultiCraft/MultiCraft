local function get_formspec(tabview, name, tabdata)
	local retval =
		"real_coordinates[true]" ..
		"bgcolor[#0000]" ..

		"container[0,0]" ..
		"button[0,0;6,1.5;solo;Singleplayer]" ..
		"button[0,1.7;6,1.5;multi;Play Online]" ..
		"button[0,3.4;6,1.5;host;Host a server]" ..
		"container_end[]" ..

		"hypertext[2.5,6;2,1;;" ..
			"<tag name=action color=#ffffed hovercolor=#ff0 font=bold size=22>" ..
			"<action name=credits>Credits</action>]"

	return retval
end

local function main_button_handler(this, fields, name, tabdata)
	--mm_texture.update("home", nil)

	if fields.solo then
		return true, switch_to_tab(this, 2)
	elseif fields.multi then
		asyncOnlineFavourites()
		return true, switch_to_tab(this, 4)
	elseif fields.host then
		return true, switch_to_tab(this, 4)
	else
		return true, switch_to_tab(this, 7)
	end
end

local function on_change(type, old_tab, new_tab)
	--mm_texture.update("home", nil)
end

--------------------------------------------------------------------------------
return {
	name = "home",
	caption = fgettext("Home"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change,
	no_bg = true,
	tabsize = {width = 6, height = 6.3},
}
