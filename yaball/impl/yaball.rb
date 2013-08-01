#!/usr/bin/env ruby
module YABALL
  class Program
    attr_reader :width,:height
    def initialize(code)
      @lines = code.split(/\r?\n|\r/)
      @width = @lines.map{|line|line.size}.max
      @lines.pop if @lines.last.empty?
      @lines.each do |line|
        if line.size<@width
          line<<(' '*(width-line.size))
        end
      end
      @height=@lines.size
    end
    def [](x,y)
      @lines[y][x]
    end
  end
  class Tape
    def initialize
      @cells=[0]
      @pos=0
    end
    def cell_value
      @cells[@pos]
    end
    def cell_value?
      @cells[@pos]!=0
    end
    def cell_value=(new_value)
      @cells[@pos]=new_value & 0xFFFF
    end
    def move_right!
      @pos+=1
      @cells << 0 unless @pos < @cells.size
    end
    def move_left!
      if @pos==0
        @cells.unshift 0
      else
        @pos-=1
      end
    end
    def cell_incr!
      @cells[@pos]+=1
      @cells[@pos]&=0xFFFF
    end
    def cell_decr!
      @cells[@pos]-=1
      @cells[@pos]&=0xFFFF
    end
  end
  class Interpreter
    def initialize(program)
      @program = program
      @x=0
      @y=0
      @exit=false
      @mode=:normal
      @tape=Tape.new
    end
    def exited?
      @exit
    end
    def exit_status
      @exit
    end
    def run
      self.step! until self.exited?
      self.exit_status
    end
    def step!
      case @mode
      when :normal
        case @program[@x,@y]
        when ?+
          @tape.cell_incr!
        when ?-
          @tape.cell_decr!
        when ?<
          @tape.move_left!
        when ?>
          @tape.move_right!
        when ?[
          if @tape.cell_value?
            @y+=1
          end
        when ?]
          @y-=1
          @mode=:reverse
        when ?.
          output(@tape.cell_value)
        when ?,
          @tape.cell_value = input
        when ??
          @mode=:reverse
        when ?^
          @y-=1
        when ?v
          @y+=1
        when ?@
          @exit = @tape.cell_value
        end
      when :reverse
        case @program[@x,@y]
        when ?!
          @mode=:normal
        when ?9
          @y-=1
        when ?6
          @y+=1
        end
      end
      move!
    end
  private
    def move!
      case @mode
      when :normal
        @x+=1
      when :reverse
        @x-=1
      end
      if @x<0
        @x=@program.width-1
        @y-=1
      elsif @x>=@program.width
        @x=0
        @y+=1
      end
      @y%=@program.height
    end
    def output(value)
      raise NotImplementedError
    end
    def input
      raise NotImplementedError
    end
  end
  module DefaultIO
  private
    begin
      require "Win32API"
      def read_char
        Win32API.new("crtdll", "_getch", [], "L").Call
      end
    rescue LoadError
      def read_char
        system "stty raw -echo"
        STDIN.getc
      ensure
        system "stty -raw echo"
      end
    end
    def input
      return -1 if STDIN.eof?
      return STDIN.getc unless @charin
      @charin=false if @charin == :once
      return read_char
    end
    def output(value)
      case value
      when 0..255
        STDOUT.putc(value)
      when 256..511
        STDERR.putc(value-256)
      when 512
        STDOUT.close
      when 513
        STDERR.close
      when 514
        STDIN.close
      when 515
        STDOUT.flush
      when 516
        STDERR.flush
      when 517
        @charin = :once
      when 518
        STDOUT.sync=true
      when 519
        STDERR.sync=true
      when 520
        @charin = true
      when 521
        STDOUT.sync=false
      when 522
        STDERR.sync=false
      when 523
        @charin = false
      end
    end
  end
end
if __FILE__ == $0
  include YABALL
  program = Program.new(ARGF.read)
  STDIN.rewind if ARGF.to_io == STDIN
  interpreter = Interpreter.new(program)
  interpreter.extend DefaultIO
  exit interpreter.run
end