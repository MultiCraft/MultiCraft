local function make_registration()
    local t = {}
    local registerfunc = function(func)
        t[#t + 1] = func
    end
    return t, registerfunc
end

core.registered_on_update, core.register_on_update = make_registration()

function core.handle_update(...)
    for _, func in ipairs(core.registered_on_update) do
        func(...)
    end
end
