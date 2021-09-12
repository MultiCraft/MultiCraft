-- CSM death formspec. Only used when clientside modding is enabled, otherwise
-- handled by the engine.

core.register_on_death(function()
	core.display_chat_message(fgettext("You died."))
	local formspec = "size[11,5.5]bgcolor[#320000b4;true]" ..
		"image_button[2,2;7,0.8;blank.png;;" .. fgettext("You died.") .. ";false;false;]" ..
		"button_exit[3.5,3;4,0.5;btn_respawn;".. fgettext("Respawn") .."]"
	core.show_formspec("bultin:death", formspec)
end)

core.register_on_formspec_input(function(formname, fields)
	if formname == "bultin:death" and fields.btn_respawn then
		core.send_respawn()
	end
end)
