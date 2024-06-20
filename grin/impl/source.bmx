SuperStrict
Framework brl.standardio
Import brl.blitz
Import brl.filesystem
Import brl.stream
Import brl.retro
Import brl.math




Global path$="code.txt"
If AppArgs.length>1 Then
	Local ona%=0
	For Local a$=EachIn AppArgs
		ona:+1
		If ona=2 Then path=a
	Next
EndIf



Global code@[1]

Local fil:TStream=ReadFile(path)
If Not fil Print "Could not load "+path;Input;End

Global size:long=1
Global cmds$="><.:,;'~q+-*/^$\~~=%sctSCT12r&|!@[]?}{()`_eplmqLjD"
Global ends%=0
Global inparen%=0
Global comment%=0
Repeat
	Local b$=Chr(ReadByte(fil))
	If (Instr(cmds,b) Or inparen) And comment=0
		If b="(" inparen=1
		If b=")" inparen=0
		code=code[..size+1]
		code[size-1]=Asc(b)
		If b="_" ends=1
		size:+1
	ElseIf b="#"
		comment=Not comment
	EndIf
Until Eof(fil)


Print "Executing program."
Print ""



Global memory:Double[64],msize%=64
Global mmod%=0
Function get:Double(x:Long)
	If x+mmod<0 Then 
		mmod=-x;memory=memory[..msize+mmod]
		For Local rsize%=0 To msize+mmod-2
			memory[msize+mmod-1-rsize]=memory[msize+mmod-2-rsize]
		Next
		memory[0]=0
	EndIf
	If x=>msize Then msize:+1;memory=memory[..msize+mmod]
	If x+mmod<0
		Print msize+mmod
		Print x+mmod
	EndIf
	Return memory[x+mmod]
End Function
Function set(x:Long,s:Double)
	get x
	memory[x+mmod]=s
End Function


Global degrees%=1
Global pos:Long=0
Global mem:Long=0
Global reg:Double=0
Global skips%=0
Repeat
	If pos=>size Then Exit
	'Print Chr(code[pos])
	If skips=0
		Select Chr(code[pos])
			Case ">" mem:+1
			Case "<" mem:-1
			Case "+" set(mem,get(mem)+reg)
			Case "-" set(mem,get(mem)-reg)
			Case "*" set(mem,get(mem)*reg)
			Case "/" set(mem,get(mem)/reg)
			Case "^" set(mem,get(mem)^reg)
			Case "%" set(mem,get(mem) Mod reg)
			Case "r" set(mem,round(get(mem)))
			Case "$" reg=get(mem)
			Case "\" set(mem,reg)
			Case "~~"Local tmp:Double=reg;reg=get(mem);set(mem,tmp)
			Case "=" reg=0
			Case "&" set(mem,lnand(get(mem),reg))
			Case "|" set(mem,lor(get(mem),reg))
			Case "!" set(mem,lnot(get(mem)))
			Case "@" set(mem,-get(mem))
			Case "." WriteStdout Chr(get(mem))
			Case ":" WriteStdout get(mem)
			Case "," set(mem,Asc(Input()))
			Case ";" set(mem,Double(Input()))
			Case "'" WriteStdout Chr(reg)
			Case "~q"WriteStdout reg
			Case "[" If get(mem)<=0 Then pos=match(pos,"[","]",1)
			Case "]" pos=match(pos,"]","[",-1)-1
			Case "?" set(mem,sign(get(mem)))
			Case "}" set(mem,get(mem)+1)
			Case "{" set(mem,get(mem)-1)
			Case "(" Local f:Long=match(pos,"",")",1);writeparen pos,f;pos=f
			Case ")" Print ""
			Case "s" set(mem,Sin(deg(get(mem))))
			Case "c" set(mem,Cos(deg(get(mem))))
			Case "t" set(mem,Tan(deg(get(mem))))
			Case "S" set(mem,ASin(deg(get(mem))))
			Case "C" set(mem,ACos(deg(get(mem))))
			Case "T" set(mem,ATan(deg(get(mem))))
			Case "1" set(mem,Double(1)/get(mem))
			Case "2" set(mem,get(mem) Mod 2)
			Case "e" set(mem,e())
			Case "p" set(mem,Pi)
			Case "l" set(mem,Log10(get(mem)))
			Case "L" set(mem,Logbx(get(mem),reg))
			Case "m" set(mem,(get(mem)+reg)/Double(2))
			Case "q" set(mem,get(mem)^(1/reg))
			Case "D" degrees=Not degrees
			Case "j" If reg>0 skips=Floor(reg)
			Case "_" set(mem,0)
			Case "`" Exit
		End Select
	Else
		skips:-1
		Select Chr(code[pos])
			Case "[" If get(mem)<=0 Then pos=match(pos,"[","]",1)
			Case "]" pos=match(pos,"]","[",-1)-1
			Case "(" Local f:Long=match(pos,"",")",1);pos=f
		End Select
	EndIf
'	Print get(mem)
	pos:+1
Forever


Print " "
Print " "
Print "Execution complete."
Input
End




Function match:Long(start:Long,inc$,dec$,dir%)
	Local nest:Long=1,in:Long=start+dir
	Repeat
		If in=>size Or in<0 
			Print "ERROR: could not find matching "+dec;Input;End
		EndIf
		If Chr(code[in])=inc nest:+1
		If Chr(code[in])=dec nest:-1
		If nest=0 Return in
		in:+dir
	Forever
End Function


Function writeparen(start:Long,fin:Long)
	Local txt$=""
	For Local write:Long=start+1 To fin-1
		txt:+Chr(code[write])
	Next
	WriteStdout txt
End Function


Function lnand%(x:Double,y:Double)
	If (x=0) | (y=0) Return 1
	Return 0
End Function
Function lor%(x:Double,y:Double)
	If (x<>0) | (y<>0) Return 1
	Return 0
End Function
Function lnot%(x:Double)
	If (x=0) Return 1
	Return 0
End Function

Function deg%(x%)
	If degrees=1 Return x
	Return x*Pi/Double(180)
End Function

Function e:Double()
	Return 2.71828182845904523536
End Function

Function Logbx:Double(x:Double,b:Double)
	Return Log(x)/Log(b)
End Function

Function round:Double(num:Double)
	Local dec:Double=num Mod 1
	If dec=>.5 Return num+(1-dec)
	Return num-dec
End Function


Function sign:Double(num:Double)
	If num>0 Return 1
	If num<0 Return -1
	Return 0
End Function