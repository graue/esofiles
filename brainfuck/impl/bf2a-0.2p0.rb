#!/usr/bin/env ruby
=begin
/*
 * BF2A -- Optimizing Brainfuck Compiler
 * usage:
 * ruby bf2a.rb program.b [output.c]
 *
 * Version 0.2p0
 * Modified by Graue to incorporate a bug fix from the author.
 *
 * Copyright (c) 2005 Jannis Harder
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
=end
module BF2A

    LUTS = []

  LUT_VALS=[85, 51, 73, 199, 93, 59, 17, 15, 229, 195, 89, 215, 237, 203, 33, 31, \
  117, 83, 105, 231, 125, 91, 49, 47, 5, 227, 121, 247, 13, 235, 65, 63, 149, \
  115, 137, 7, 157, 123, 81, 79, 37, 3, 153, 23, 45, 11, 97, 95, 181, 147, 169, \
  39, 189, 155, 113, 111, 69, 35, 185, 55, 77, 43, 129, 127, 213, 179, 201, 71, \
  221, 187, 145, 143, 101, 67, 217, 87, 109, 75, 161, 159, 245, 211, 233, 103, \
  253, 219, 177, 175, 133, 99, 249, 119, 141, 107, 193, 191, 21, 243, 9, 135, \
  29, 251, 209, 207, 165, 131, 25, 151, 173, 139, 225, 223, 53, 19, 41, 167, \
  61, 27, 241, 239, 197, 163, 57, 183, 205, 171] #they are correct.. don't ask me why
  
  LUT_VALS.each do |e|
    LUTS << []
    256.times do |n|
      LUTS.last<<((n*e)&0xFF)
    end
  end

  class BFCode
    def initialize(code,in_loop=false)
      code=code.tr("^[]<>,.+-"<<(in_loop ? 'z':''),"")
      @in_loop=in_loop
      code.gsub!(/\[(\+(\+\+)*|-(--)*)\]/,'z')
      #p code
      raw_code_parts=code.scan(/[\[\]]|[^\[\]]+/)
      @code_parts=[]
      il=0
      ts=""
      raw_code_parts.each do |cp|
        case cp
        when ']'
          il-=1
          if il<0
            raise
          elsif il==0
            @code_parts << BFCode.new(ts,true)
            ts=""
          else
            ts<<cp
          end
        when '[' 
          if il>0
            ts<<cp
          end
          il+=1
        else
          if il==0
            @code_parts << cp
          else
            ts << cp
          end
        end
      end
      raise unless il==0
      @code_parts.map do |cp|
        if cp.is_a?String
          while cp.gsub!(/<>|><|\+-|-\+/,'')
          end
          #cp.split('')
        else
          cp
        end
      end 
    end
    def movement
      count = 0
      @code_parts.each do |cp|
        if cp.is_a?String
          count+=cp.tr('^>','').size-cp.tr('^<','').size
        else
          if cp.movement!=0
            return nil
          end
        end
      end
      count
    end
    def getadds(code,offset)
      adds = Hash.new(0)
      code.each_byte do |b|
        case b
        when ?<
          offset-=1
        when ?>
          offset+=1
        when ?+
          begin
            adds[offset]+=1
          rescue
            adds[offset][0]+=1
          end
        when ?-
          begin
            adds[offset]-=1
          rescue
            adds[offset][0]-=1
          end
        when ?z
          adds[offset]=[0]
        else
          raise
        end
      end
      adds.keys.each do |e|
        if adds[e].is_a?Array
          adds[e]=[adds[e][0]&0xFF]
        else
          adds[e]&=0xFF
        end
      end
      adds.delete_if{|k,v|v==0}
      [adds,offset]
    end
    def gencode(offset=0)
      out = []
      op = out
      #done=false
      oo=offset
      if @in_loop
        #special fast optimisations
        if @code_parts.size == 1
          if not @code_parts[0].is_a? String
            return @code_parts[0].gencode(offset) #loop in loop 0o
          elsif self.movement==0 and (not(@code_parts[0].include?('.') or @code_parts[0].include?(',')))
            part = @code_parts[0]
            adds, = getadds(part,offset)
            ao = adds[offset]
            #p ao
            if (ao.is_a?(Array) and ao[0]!=0) or (not ao)
              return [[:inf_loop,offset]]
            elsif ao.is_a?(Array)
              op = []
              adds.each_pair do |ao,av|
                if av.is_a?Array
                  op<< [:set,ao,av[0]]
                else
                  av&=0xFF
                  
                  op<< [:add,ao,av] if av!=0
                end
              end
              return [[:if,offset,op]]
            elsif adds.size==1 && ao & 1 == 0 # i could optimize with adds.size!=1 but i'm too lazy
              c = 0
              aoc=ao
              while aoc & 1 == 0
                aoc>>=1
                c+=1
              end
              return [[:if_divtxilseif,offset,c,0]] ## do stage 3 things with this!
            elsif ao & 1 == 1 #mov_mul!
              #p :yes
              opd = []
              ops = []
              lut = (ao >> 1)-1
              adds.delete(offset)
              adds.each_pair do |(ao,av)|
                if av.is_a? Array
                  ops << [ao,av[0]]
                else
                  opd << [ao,av]
                end
              end
              op=:mov_mul
              case lut
              when -1
                op = :p_mov_mul
              when 126
                op = :n_mov_mul
              end
              return [[op,offset,lut,opd,ops]]
            end
            #p ao
          end
        end
      end
      #unless done
      # i have to add some [......[-](+)?] optimizations (oh i'm doing them but not for all loops.. should move the code
      # to v v v...)
        if @in_loop
          if self.movement == 0
            op << [:sloop,offset,[]]
          else
            op << [:loop,offset,[]]
          end
          op = op.last.last
        end
        @code_parts.each do |e|
          if e.is_a?String
            parts = e.split(/(\.|\,)/)
            parts.delete_if{|e|e.empty?}
            parts.each do |part|
              if part=='.'
                op<< [:putc,offset]
              elsif part==','
                op<< [:getc,offset]
              else
                adds,offset = getadds(part,offset)
                #p part,adds
                adds.each_pair do |ao,av|
                  if av.is_a?Array
                    op<< [:set,ao,av[0]]
                  else
                    op<< [:add,ao,av] if av!=0
                  end
                end
              end
            end
          else
            op.replace(op.concat(e.gencode(offset)))
          end
        end
        if @in_loop
          unless offset==oo
            op << [:move,offset-oo]
          end
      #end
      else 
        out = stage3(out)
      end
      out
    end
    def stage3(a_code) # write me!
      a_code
    end
  end
  class A2C
    def initialize(a_code)
      @a_code=a_code

    end
=begin
    def get_cpvar
      var = (@cpvars_free.delete(@cpvars_free.keys.first) || "var_#{@lgc.succ!}")
      @cpvars_in_use[var]=true
      var
    end
    def free_cpvar(var)
      @cpvars_in_use.delete(var)
      @cpvars_free[var]=true
    end
    def get_var
      var = (@vars_free.delete(@vars_free.keys.first) || "var_#{@lgc.succ!}")
      @vars_in_use[var]=true
      var
    end
    def free_var(var)
      @vars_in_use.delete(var)
      @vars_free[var]=true
    end
=end
    def adjust_mm(v)
      if v<@min_g
        @min_g=v
      elsif v>@max_g
        @max_g=v
      end
    end
    def adjust_pm(v)
      if v<@min_p
        @min_p=v
      elsif v>@max_p
        @max_p=v
      end
    end
    def gencode
      #@vars_in_use={}
      #@vars_free={}
      #@cpvars_in_use={}
      #@cpvars_free={}
      @luts_used={}
      @max_g=@max_p =0
      @min_g=@min_p =0
      @lgc="a"
      rgcode = recgen(@a_code)
      @max_g+=@max_p+1
      @min_g+=@min_p-1
      
      luts = ""
      @luts_used.keys.sort.each do |lut|
        luts << "const unsigned char lut_#{lut}[] = {#{LUTS[lut].join(', ')}};\n"
      end
      out = <<"EOH"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define REALLOC if(m>=m_max){ \\
                  m_p = m-m_srt; \\
                  m_srt = (unsigned char*)realloc(m_srt,m_size<<2); \\
                  memset(m_srt+m_size,0,m_size); \\
                  m_size<<=2; \\
                  m=m_srt+m_p; \\
                  m_min = m_srt+#{-@min_g}; \\
                  m_max = m_min+m_size-#{@max_g+300}; \\
                }
                
#{luts}
                
int main(int argc,int *argv[]){
  unsigned char * m;
  unsigned char * m_srt;
  unsigned char * m_min;
  unsigned char * m_max;
  int    m_size;
  int    m_p;
  unsigned char v,f;
  
  m_srt = (unsigned char*)malloc(#{-@min_g+@max_g+3000});
  memset(m_srt,0,#{-@min_g+@max_g+3000});
  m_min = m = m_srt+#{-@min_g};
  m_max = m_min+2700;
  m_size = #{-@min_g+@max_g+3000};
EOH
      out << rgcode
      out << "}\n\n"
    end
    def recgen(code)
      out = ""
      code.each do |i|
        case i.first
        when :getc
          out << "m[#{i[1]}]=getchar();\n"
          adjust_mm i[1]
        when :putc
          out << "putchar(m[#{i[1]}]);\n"
          adjust_mm i[1]
        when :add
          out << "m[#{i[1]}]+=#{i[2]};\n"
          adjust_mm i[1]
        when :move
          out << "m+=#{i[1]};\n"
          out << "REALLOC;\n" if i[1]>0
          adjust_pm i[1]
        when :loop,:sloop
          out << "while(m[#{i[1]}]){\n"
          out << recgen(i[2])
          out << "}\n"
          adjust_mm i[1]
        when :inf_loop
          out << "if(m[#{i[1]}]) while(1){sleep(-1);};\n"
          adjust_mm i[1]
        when :set
          out << "m[#{i[1]}]=#{i[2]};\n"
          adjust_mm i[1]
        when :if
          out << "if(m[#{i[1]}]){\n"
          out << recgen(i[2])
          out << "}\n"
          adjust_mm i[1]
        when :if_divtxilseif
          out << "if(m[#{i[1]}]&#{(1<<i[2])-1}) while(1){sleep(-1);};\n"
          out << "else m[#{i[1]}]=#{i[3]};\n"
          adjust_mm i[1]
        when :mov_mul
          @luts_used[i[2]]=true
          es = (i[3].size>3 or i[4].size>0)
          us = (i[3].size>1)
          out << "v = m[#{i[1]}];\n" 
          out << "if(v){\n" if es
          out << "f = lut_#{i[2]}[v];\n" if us
          i[3].each do |opd|
            if us
              out << "m[#{opd[0]}]+=f#{opd[1]!=1 ? "*#{opd[1]}" : ''};\n"
            else
              out << "m[#{opd[0]}]+=lut_#{i[2]}[v]#{opd[1]!=1 ? "*#{opd[1]}" : ''};\n"
            end
            adjust_mm opd[0]
          end
          i[4].each do |ops|
            out << "m[#{ops[0]}]=#{ops[1]};\n"
            adjust_mm ops[0]
          end
          out << "m[#{i[1]}]=0;\n"
          out << "};\n" if es
          adjust_mm i[1]
        when :p_mov_mul
          es = (i[3].size>3 or i[4].size>0)
          us = (i[3].size>1)
          out << "v = m[#{i[1]}];\n" if us
          out << "if(v){\n" if es
          i[3].each do |opd|
            if us
              out << "m[#{opd[0]}]-=v#{opd[1]!=1 ? "*#{opd[1]}" : ''};\n"
            else
              out << "m[#{opd[0]}]-=m[#{i[1]}]#{opd[1]!=1 ? "*#{opd[1]}" : ''};\n"
            end
            adjust_mm opd[0]
          end
          i[4].each do |ops|
            out << "m[#{ops[0]}]=#{ops[1]};\n"
            adjust_mm ops[0]
          end
          out << "m[#{i[1]}]=0;\n"
          out << "};\n" if es
          adjust_mm i[1]
        when :n_mov_mul
          es = (i[3].size>3 or i[4].size>0)
          us = (i[3].size>1)
          out << "v = m[#{i[1]}];\n"
          out << "if(v){\n" if es
          i[3].each do |opd|
            if us
              out << "m[#{opd[0]}]+=v#{opd[1]!=1 ? "*#{opd[1]}" : ''};\n"
            else
              out << "m[#{opd[0]}]+=m[#{i[1]}]#{opd[1]!=1 ? "*#{opd[1]}" : ''};\n"
            end
            adjust_mm opd[0]
          end
          i[4].each do |ops|
            out << "m[#{ops[0]}]=#{ops[1]};\n"
            adjust_mm ops[0]
          end
          out << "m[#{i[1]}]=0;\n"
          out << "};\n" if es
          adjust_mm i[1]
        else
          raise
        end
      end
      out
    end
  end
end
if __FILE__ == $0
  include BF2A
  e = nil
  infile = File.open(e=ARGV.shift,"r") rescue nil
  infile ||= STDIN
  outfile = File.open(ARGV.shift,"w") rescue nil
  outfile ||= File.open(File.basename(e).sub(/\.bf?$/,'')+'.c',"w") rescue nil
  outfile ||= STDOUT
  
  bf_code = BFCode.new(infile.read)
  a_code = bf_code.gencode
  #STDERR.puts a_code.inspect
  c_code = A2C.new(a_code).gencode
  outfile.write c_code
end
