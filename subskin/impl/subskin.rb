m=readlines.map{|e|e.hex};loop{m[1]<0||$><<m[1].chr&&m[1]=-1;m[2]<0&&m[2]=STDIN.getc||256;a,b,c=m[m[0],3];q=(m[c]=m[a]-m[b])<0?6:3;m[0]+=q}rescue 0
