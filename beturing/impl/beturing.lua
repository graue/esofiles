#!/usr/local/bin/lua
--
-- beturing.lua v1.1
-- Interpreter for Beturing v1.1
-- A Befunge-flavoured Turing machine
-- Implemented in Lua 5 by Chris Pressey, June 2005
--

--
-- Copyright (c)2005 Cat's Eye Technologies.  All rights reserved.
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
--
--   Redistributions of source code must retain the above copyright
--   notice, this list of conditions and the following disclaimer.
--
--   Redistributions in binary form must reproduce the above copyright
--   notice, this list of conditions and the following disclaimer in
--   the documentation and/or other materials provided with the
--   distribution.
--
--   Neither the name of Cat's Eye Technologies nor the names of its
--   contributors may be used to endorse or promote products derived
--   from this software without specific prior written permission.
--
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
-- ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
-- LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
-- FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
-- COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
-- INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
-- (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
-- SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
-- HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
-- STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
-- OF THE POSSIBILITY OF SUCH DAMAGE. 
--
-- $Id$

--
-- v1.0: June 6 2005: initial release
-- v1.1: June 8 2005: changed semantics of '*' special code
--

--[[ Common functions ]]--

local debug_log = print
local usage = function()
    io.stderr:write("Usage: [lua] beturing.lua [-oq] [-d x1,y1:x2,y2] [filename.bet]\n")
    os.exit(1)
end
local old_semantics = false
local display = nil

--[[ Object Classes ]]--

--[[-----------]]--
--[[ Playfield ]]--
--[[-----------]]--

--
-- Store an unbounded grid.
--
Playfield = {}
Playfield.new = function(tab)
    tab = tab or {}
    local nw, ne, sw, se = {}, {}, {}, {}  -- quadrant storage
    local min_x, min_y, max_x, max_y       -- limits seen so far
    local method = {}

    --
    -- Private function: pick the appropriate quadrant & translate
    --
    local pick_quadrant = function(x, y)
        if x >  0 and y >  0 then return se, x, y end
        if x >  0 and y <= 0 then return ne, x, 1-y end
        if x <= 0 and y >  0 then return sw, 1-x, y end
        if x <= 0 and y <= 0 then return nw, 1-x, 1-y end
    end

    --
    -- Read the symbol at a given position in the playfield
    --
    method.peek = function(pf, x, y)
        local contents, nx, ny = pick_quadrant(x, y)
        contents[ny] = contents[ny] or {} -- make sure row exists
        local sym = contents[ny][nx] or " "
	return sym
    end

    --
    -- Write a symbol at a given position in the playfield
    --
    method.poke = function(pf, x, y, sym)
        local contents, nx, ny = pick_quadrant(x, y)
        contents[ny] = contents[ny] or {} -- make sure row exists
        contents[ny][nx] = sym
	if not min_x or x < min_x then min_x = x end
	if not max_x or x > max_x then max_x = x end
	if not min_y or y < min_y then min_y = y end
	if not max_y or y > max_y then max_y = y end
    end

    --
    -- Store a string starting at (x, y).
    --
    method.poke_str = function(pf, x, y, str)
	local i
	for i = 1, string.len(str) do
	    pf:poke(x + (i - 1), y, string.sub(str, i, i))
	end
    end

    --
    -- Load the playfield from a file.
    --
    method.load = function(pf, filename, callback)
        local file = io.open(filename)
	local line = file:read("*l")
	local x, y = 0, 0

        while line do
	    if string.find(line, "^%s*%#") then
	        -- comment or directive - not included in playfield.
		local found, len, nx, ny =
		  string.find(line, "^%s*%#%s*%@%(%s*(%-?%d+)%s*%,%s*(%-?%d+)%s*%)")
		if found then
		    x = tonumber(nx)
		    y = tonumber(ny)
		    debug_log("Now loading at " ..
		      "(" .. tostring(x) .. "," .. tostring(y) .. ")")
		else
		    callback(line)
		end
	    else
	        pf:poke_str(x, y, line)
		y = y + 1
	    end
            line = file:read("*l")
	end
	file:close()
    end

    --
    -- Return a string representing the playfield.
    --
    method.render = function(pf, start_x, start_y, end_x, end_y)
	start_x = start_x or min_x
        start_y = start_y or min_y
	end_x = end_x or max_x
	end_y = end_y or max_y
        local y = start_y
	local s = "--- (" .. tostring(start_x) .. "," .. tostring(start_y) .. ")-"
	s = s .. "(" .. tostring(end_x) .. "," .. tostring(end_y) .. ") ---\n"
        while y <= end_y do
	    local x = start_x
	    while x <= end_x do
	        s = s .. pf:peek(x, y)
	        x = x + 1
	    end
	    s = s .. "\n"
            y = y + 1
	end

	return s
    end

    return method
end

--[[------]]--
--[[ Head ]]--
--[[------]]--

--
-- Represent a readable(/writeable) location within a playfield.
--
Head = {}
Head.new = function(tab)
    tab = tab or {}

    local pf = assert(tab.playfield)
    local x = tab.x or 0
    local y = tab.y or 0
    local moves_left = 0
    local moves_right = 0

    local method = {}

    method.report = function(hd)
        io.stdout:write("Moves left:  " .. tostring(moves_left) .. ", moves right: " .. tostring(moves_right) .. "\n")
	io.stdout:write("Total moves: " .. tostring(moves_left + moves_right) .. "\n")
    end

    method.read = function(hd, sym)
        return pf:peek(x, y)
    end

    method.write = function(hd, sym)
        pf:poke(x, y, sym)
    end

    --
    -- look for this symbol         -> 13 <- on match, write this symbol
    -- on match, move head this way -> 24 <- choose next state on this
    --
    method.read_code = function(hd)
        local seek_sym, repl_sym, move_cmd, state_cmd

	debug_log("rd cd")
	seek_sym = hd:read()
	hd:move(">")
	repl_sym = hd:read()
	hd:move("v")
	state_cmd = hd:read()
	hd:move("<")
	move_cmd = hd:read()
	hd:move("^")
	debug_log("cd rd")
	
	return seek_sym, repl_sym, move_cmd, state_cmd
    end

    method.move = function(hd, sym)
        if sym == "^" then
            y = y - 1
        elseif sym == "v" then
            y = y + 1
        elseif sym == "<" then
            x = x - 1
	    moves_left = moves_left + 1
        elseif sym == ">" then
            x = x + 1
	    moves_right = moves_right + 1
        elseif sym ~= "." then
            error("Illegal movement symbol '" .. sym .. "'")
        end
    end

    return method
end

--[[---------]]--
--[[ Machine ]]--
--[[---------]]--

--
-- Perform the mechanics of the machine.
--
Machine = {}
Machine.new = function(tab)
    tab = tab or {}

    local pf = tab.playfield or Playfield.new()
    local data_head = Head.new{
        playfield = pf,
        x = tab.data_head_x or 0,
	y = tab.data_head_y or 0
    }
    local code_head = Head.new{
        playfield = pf,
        x = tab.code_head_x or 0,
	y = tab.code_head_y or 0
    }

    local method = {}

    --
    -- Private function: provide interpretation of the state-
    -- transition operator.
    --
    local interpret = function(sym, sense)
        if sense then
	    -- Positive interpretation.
	    -- Backwards compatibility:
	    if old_semantics then
	        if sym == "/" then
		    return ">"
		else
		    return sym
		end
	    end
	    if sym == "/" or sym == "`" then
		return ">"
	    elseif sym == "\\" or sym == "'" or sym == "-" then
		return "<"
	    elseif sym == "|" then
		return "^"
	    else
	        return sym
	    end
	else
	    -- Negative interpretation.
	    -- Backwards compatibility:
	    if old_semantics then
	        if sym == "/" then
		    return "v"
		else
		    return sym
		end
	    end
	    if sym == "/" or sym == "\\" or sym == "|" then
		return "v"
	    elseif sym == "-" then
		return ">"
	    elseif sym == "`" or sym == "'" then
		return "^"
	    else
	        return state_cmd
	    end
	end
    end

    --
    -- Advance the machine's configuration one step.
    --
    method.step = function(m)
        local this_sym = data_head:read()
        local seek_sym, repl_sym, move_cmd, state_cmd = code_head:read_code()
	local code_move

	debug_log("Symbol under data head is '" .. this_sym .. "'")
	debug_log("Instruction under code head is:")
	debug_log("(" .. seek_sym .. repl_sym .. ")")
	debug_log("(" .. move_cmd .. state_cmd .. ")")

	--
	-- Main processing logic
	--
	if move_cmd == "*" then
	    --
	    -- Special - match anything, do no rewriting,
	    -- move the data head using the replacement symbol
	    -- (unless using the old compatibility semantics,)
	    -- and advance the state using the positive interpretation.
	    --
	    debug_log("-> Wildcard!")
	    if not old_semantics then
	        data_head:move(repl_sym)
	    end
	    code_move = interpret(state_cmd, true)
	elseif seek_sym == this_sym then
	    --
	    -- The seek symbol matches the symbol under the data head.
	    -- Rewrite it, move the head, and advance the state
	    -- using the positive interpretation.
	    --
	    debug_log("-> Symbol matches, replacing with '" .. repl_sym .. "'")
	    debug_log("-> moving data head '" .. move_cmd .. "'")
	    data_head:write(repl_sym)
	    data_head:move(move_cmd)
	    code_move = interpret(state_cmd, true)
	else
	    --
	    -- No match - just advance the state, using negative interp.
	    --
	    debug_log("-> No match.")
	    code_move = interpret(state_cmd, false)
	end

	--
	-- Do the actual state advancement here.
	--
	if code_move == "@" then
	    debug_log("-> Machine halted!")
	    return false
	else
            debug_log("-> moving code head '" .. code_move .. "'")
            code_head:move(code_move)
	    code_head:move(code_move)
	    return true
	end
    end

    --
    -- Run the machine 'til it halts.
    --
    method.run = function(m)
	local done = false
	while not done do
	    if display then
		io.stdout:write(pf:render(display.x1, display.y1,
		                          display.x2, display.y2))
	    else
	        debug_log(pf:render())
	    end
	    done = not m:step()
	end
	data_head:report()
    end

    return method
end

--[[ INIT ]]--

local pf = Playfield.new()

--[[ command-line arguments ]]--

local argno = 1
while arg[argno] and string.find(arg[argno], "^%-") do
    if arg[argno] == "-q" then          -- quiet
        debug_log = function() end
    elseif arg[argno] == "-o" then      -- use v1.0 semantics
        old_semantics = true
    elseif arg[argno] == "-d" then
        argno = argno + 1
	local found, len
	display = {}
        found, len, display.x1, display.y1, display.x2, display.y2 =
	    string.find(arg[argno], "(%-?%d+)%,(%-?%d+)%:(%-?%d+)%,(%-?%d+)")
	if not found then
	    usage()
	end
	display.x1 = tonumber(display.x1)
	display.y1 = tonumber(display.y1)
	display.x2 = tonumber(display.x2)
	display.y2 = tonumber(display.y2)	
    else
        usage()
    end
    argno = argno + 1
end

if not arg[argno] then
    usage()
end

--[[ load playfield ]]--

local data_head_x, data_head_y, code_head_x, code_head_y = 0, 0, 0, 0
local directive_processor = function(directive)
    local found, len, x, y

    found, len, x, y = 
      string.find(directive, "^%s*%#%s*D%(%s*(%-?%d+)%s*%,%s*(%-?%d+)%s*%)")
    if found then
        data_head_x = tonumber(x)
	data_head_y = tonumber(y)
	debug_log("Data head initially located at " ..
          "(" .. tostring(data_head_x) .. "," .. tostring(data_head_y) .. ")")
	return true
    end
    found, len, x, y = 
      string.find(directive, "^%s*%#%s*C%(%s*(%-?%d+)%s*%,%s*(%-?%d+)%s*%)")
    if found then
        code_head_x = tonumber(x)
	code_head_y = tonumber(y)
	debug_log("Code head initially located at " ..
          "(" .. tostring(code_head_x) .. "," .. tostring(code_head_y) .. ")")
	return true
    end

    return false
end

pf:load(arg[argno], directive_processor)

--[[ MAIN ]]--

local m = Machine.new{
    playfield = pf,
    data_head_x = data_head_x,
    data_head_y = data_head_y,
    code_head_x = code_head_x,
    code_head_y = code_head_y
}

m:run()
