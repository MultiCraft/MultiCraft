local function get_formspec(tabview, name, tabdata)
	local retval =
		"real_coordinates[true]" ..
		"style_type[button;border=false;content_offset=0]" ..
		"style_type[image_button;border=false;bgimg_hovered=" ..
			core.formspec_escape(defaulttexturedir .. "select.png") .. "]" ..

		"bgcolor[#0000]" ..
		"container[3,4.5]" ..

		"image_button[0,0;3,3;" .. core.formspec_escape(defaulttexturedir .. "hosting_create.png") .. ";solo;]" ..
		"button[0,3.1;3,0.5;__;Singleplayer]" ..

		"image_button[3.5,0;3,3;" .. core.formspec_escape(defaulttexturedir .. "hosting_create.png") .. ";multi;]" ..
		"button[3.5,3.1;3,0.5;__;Multiplayer]" ..

		"image_button[7,0;3,3;" .. core.formspec_escape(defaulttexturedir .. "hosting_create.png") .. ";host;]" ..
		"button[7,3.1;3,0.5;__;Server Hosting]" ..

		"container_end[]" ..

		"hypertext[14.8,11.5;2,1;;" ..
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
	elseif not fields["__"] then
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
	tabsize = {width = 16, height = 12},
}
