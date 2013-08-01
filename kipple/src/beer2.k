# Alternative, more compact version of "99 bottles of beer"

99>n

(n-1
  10>o " of beer on the wall">o b-1 b>a? (a "s">o 0>a?) " bottle">o b+0 b>@ (@>o)  
  10>o "Take one down and pass it around">o b+1  
  10>o " of beer">o             b-1 b>a? (a "s">o 0>a?) " bottle">o b+0 b>@ (@>o) 
  10>o " of beer on the wall">o b-1 b>a? (a "s">o 0>a?) " bottle">o b+0 b>@ (@>o)
  10>o n?  
)