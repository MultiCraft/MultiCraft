-- CSM death formspec. Only used when clientside modding is enabled, otherwise
-- handled by the engine.

core.register_on_death(function()
	core.display_chat_message(fgettext("You died"))
	local formspec = "size[8,5]bgcolor[#320000b4;true]" ..
		"background9[0,0;0,0;bg_common.png;true;40]" ..
		"style[you_died;font_size=+4;content_offset=0]" ..
		"image_button[0.5,1.5;7,0.8;blank.png;you_died;" .. fgettext("You died") .. ";false;false]" ..
		btn_style("btn_respawn") ..
		"button_exit[2,3;4,0.5;btn_respawn;".. fgettext("Respawn") .."]"
	core.show_formspec("bultin:death", formspec)
end)

core.register_on_formspec_input(function(formname, fields)
	if formname == "bultin:death" and not fields.you_died then
		core.send_respawn()
	end
end)
