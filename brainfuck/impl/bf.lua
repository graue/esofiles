-- Brainfuck interpreter in Lua. This is public domain.
local src, err = io.open(arg[1], "r")
if src == nil then
    print("Error opening file " .. arg[1] .. ": " .. err)
    os.exit(1)
end
local code = src:read("*a")
src:close()
src = nil

local ip = 1
local dp = 1
local mem = {0}
local codelen = string.len(code)

local commands =
{
    add = string.byte("+"),
    sub = string.byte("-"),
    next = string.byte(">"),
    prev = string.byte("<"),
    startloop = string.byte("["),
    endloop = string.byte("]"),
    input = string.byte(","),
    output = string.byte(".")
}

while ip <= codelen do
    local cmd = string.byte(code, ip)
    if cmd == commands.add then
        mem[dp] = mem[dp] + 1
        if mem[dp] == 256 then mem[dp] = 0 end
    elseif cmd == commands.sub then
        mem[dp] = mem[dp] - 1
        if mem[dp] == -1 then mem[dp] = 255 end
    elseif cmd == commands.next then
        dp = dp + 1
        if mem[dp] == nil then mem[dp] = 0 end
    elseif cmd == commands.prev then
        dp = dp - 1
        if dp == 0 then
            print("Underflow error at " .. ip)
            os.exit(1)
        end
    elseif cmd == commands.input then
        local entry = io.stdin:read(1)
        if entry == nil then
            mem[dp] = 0 -- end of file
        else
            entry = string.byte(entry)
            if entry > 255 then entry = 255
            elseif entry < 0 then entry = 0 end
            mem[dp] = entry
        end
    elseif cmd == commands.output then
        io.stdout:write(string.char(mem[dp]))
    elseif cmd == commands.startloop and mem[dp] == 0 then
        local descent = 1
        repeat
            ip = ip + 1
            if ip > codelen then
                print("Unmatched [")
                os.exit(1)
            end
            cmd = string.byte(code, ip)
            if cmd == commands.startloop then descent = descent + 1
            elseif cmd == commands.endloop then descent = descent - 1 end
        until descent == 0
    elseif cmd == commands.endloop and mem[dp] ~= 0 then
        local descent = 1
        repeat
            ip = ip - 1
            if ip == 0 then
                print("Unmatched ]")
                os.exit(1)
            end
            cmd = string.byte(code, ip)
            if cmd == commands.endloop then descent = descent + 1
            elseif cmd == commands.startloop then descent = descent - 1 end
        until descent == 0
    end
    ip = ip + 1
end
