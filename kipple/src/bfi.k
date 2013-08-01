# Brainfuck interpreter. Input is seperated from the source with 
# an ! character. Cell size is 8 bits.
#
# Brainfuck example:  
#
#  >,[>,]<[.<]!Hello. 
#
# This program (the part before the !) will output it's input
# (the part after the !) in reverse


(i>a)
1>x

# Loop that reads the source code into t, ignoring invalid 
# characters, until an ! is encountered:
(x  #while(!x.empty())  
  0>b
  
  a-43 a>z? 0>y (z y? 0>z?) (y? a>t b?) #if a.peek()=='+' t.push(a.pop())
  
  a-44 a>z? 0>y (z y? 0>z?) (y? a>t b?) 
  
  a-45 a>z? 0>y (z y? 0>z?) (y? a>t b?) 
  
  a-46 a>z? 0>y (z y? 0>z?) (y? a>t b?) 

  a-60 a>z? 0>y (z y? 0>z?) (y? a>t b?) 

  a-62 a>z? 0>y (z y? 0>z?) (y? a>t b?) 
  
  a-91 a>z? 0>y (z y? 0>z?) (y? a>t b?) 

  a-93 a>z? 0>y (z y? 0>z?) (y? a>t b?) 
  
  (b a>z b?) # discard characters that aren't Brainfuck operators
  
  # if current character is ! then stop
  a-33 a>x? x>y?
  (y
    a+0
    a>x?
    0>y?
  )
  
)  

#Move the source code to s
(t>s)



a>z # discard the ! character
(a>b) (b>i) # move the input to i

(s

  0>z? #empty z
  
  # +
  s-43
  s>x? 0>y (x y? 0>x?)
  (y? 
    d>z+1 
    z-256 z? z>q #emulate 8 bit cells 
    z>d
  )
  
  # -
  s-45
  s>x? 0>y (x y? 0>x?)
  (y?
    d>z+0
    z>q? 0>r (q r? 0>q? z-1)
    (r z+255 r?) #emulate 8 bit cells
    z>d
  )
  
  # ,
  s-44
  s>x? 0>y (x y? 0>x?)
  (y? d>z i>d)
    
  # .
  s-46
  s>x? 0>y (x y? 0>x?)
  (y? d+0 d>p)
  
  # <
  s-60
  s>x? 0>y (x y? 0>x?)
  (y? d<e)
  
  # >
  s-62
  s>x? 0>y (x y? 0>x?)
  (y? d>e)
  
  # [
  s-91
  s>x? 0>y (x y? 0>x?)
  (y
    d+0 d>m? 0>n (m n? 0>m?)
    (n
     1>b
     (b
       s>t
       s-91  s>x? 0>y (x y? 0>x?)  (y? b+1)
       s-93  s>x? 0>y (x y? 0>x?)  (y? b-1)       
       b?
     )
     n? 
    )    
    y?
  )
  
  # ]
  s-93
  s>x? 0>y (x y? 0>x?)
  (y
    d+0 d>n?
    (n
      1>b
      (b
        s<t
        s-91  s>x? 0>y (x y? 0>x?)  (y? b-1)
        s-93  s>x? 0>y (x y? 0>x?)  (y? b+1)       
        b?
      )
      0>n? 
    )    
    y?
  )    
  
  s>t   
)

(p>o)