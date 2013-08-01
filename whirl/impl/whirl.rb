## Whirl interpreter in Ruby (2005-07-09)
## Copyright (c) 2005, Kang Seonghoon (Tokigun).
## 
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public
## License as published by the Free Software Foundation; either
## version 2.1 of the License, or (at your option) any later version.
## 
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## Lesser General Public License for more details.
## 
## You should have received a copy of the GNU Lesser General Public
## License along with this library; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

class WhirlMemory
	attr_accessor :memory, :current

	def initialize
		@memory = Hash.new(0)
		@current = 0
	end

	def get
		@memory[@current]
	end
	
	def set(value)
		@memory[@current] = value
	end

	def jump(offset)
		@current += offset
	end
end

class WhirlProgram
	attr_reader :program
	attr_accessor :current

	def initialize(program)
		@program = program.delete "^01"
		@current = 0
	end

	def get
		if @current < @program.length
			code = @program[@current].chr.to_i
			@current += 1
			return code
		else
			throw :done
		end
	end

	def jump(offset)
		@current += offset
		@current = 0 if @current < 0
	end
end

class WhirlRing
	attr_accessor :value, :index, :direction

	def initialize(memory, program)
		@memory = memory
		@program = program
		@value = @index = 0
		@direction = 1
	end

	def rotate
		@index += @direction
		@index %= self.class::TABLE.length
	end

	def reverse
		@direction *= -1
	end

	def invoke
		send self.class::TABLE[@index]
	end
end

class WhirlOpRing < WhirlRing
	TABLE = [:noop, :exit, :one, :zero, :load, :store, :padd, :dadd, :logic, :if, :intio, :ascio]

	def noop
		# do nothing
	end

	def exit
		throw :done
	end

	def one
		@value = 1
	end

	def zero
		@value = 0
	end

	def load
		@value = @memory.get
	end

	def store
		@memory.set @value
	end
	
	def padd
		@program.jump @value-1
	end

	def dadd
		@memory.jump @value
	end

	def logic
		if @memory.get != 0 and @value != 0
			@value = 1
		else
			@value = 0
		end
	end

	def if
		@program.jump @value-1 if @memory.get != 0
	end

	def intio
		if @value == 0
			str = ''
			while str !~ /^(\s*-?\d+)?\D$/ do
				char = STDIN.getc
				break if char.nil?
				str << char
			end
			@memory.set str.to_i
		else
			print @memory.get
		end
	end

	def ascio
		if @value == 0
			char = STDIN.getc
			@memory.set char if not char.nil?
		else
			putc @memory.get
		end
	end
end

class WhirlMathRing < WhirlRing
	TABLE = [:noop, :load, :store, :add, :mult, :div, :zero, :lt, :gt, :eq, :not, :neg]

	def noop
		# do nothing
	end

	def load
		@value = @memory.get
	end

	def store
		@memory.set @value
	end
	
	def add
		@value += @memory.get
	end

	def mult
		@value *= @memory.get
	end

	def div
		@value /= @memory.get
	end

	def zero
		@value = 0
	end

	def lt
		if @value < @memory.get
			@value = 1
		else
			@value = 0
		end
	end

	def gt
		if @value > @memory.get
			@value = 1
		else
			@value = 0
		end
	end

	def eq
		if @value == @memory.get
			@value = 1
		else
			@value = 0
		end
	end

	def not
		if @value == 0
			@value = 1
		else
			@value = 0
		end
	end

	def neg
		@value *= -1
	end
end

class WhirlContext
	attr_accessor :memory, :program
	attr_reader :ring

	def initialize(program)
		@program = WhirlProgram.new program
		@memory = WhirlMemory.new
		@ring = [WhirlOpRing.new(@memory, @program), WhirlMathRing.new(@memory, @program)]
		@current = 0
		@previous = false
	end

	def step
		if @program.get == 0
			@ring[@current].reverse
			if @previous
				@ring[@current].invoke
				@current ^= 1
				@previous = false
			else
				@previous = true
			end
		else
			@ring[@current].rotate
			@previous = false
		end
	end

	def execute
		catch :done do
			loop { step }
		end
	end
end

unless ARGV.length == 1
	STDERR.puts "TokigunStudio Whirl interpreter in Ruby, by Kang Seonghoon"
	STDERR.puts "Usage: #{$0} <file>"
	exit 1
end

begin
	if ARGV[0] == "-"
		source = $stdin
	else
		source = File.new(ARGV[0], "r")
	end

	program = source.readlines.join
	context = WhirlContext.new(program)
	context.execute
rescue Errno::ENOENT
	STDERR.puts "Error: cannot read a file."
	exit 1
end

## vim: ts=4 sw=4
