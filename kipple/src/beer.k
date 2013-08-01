# Prints the lyrics to the famous "99 bottles of beer" song

99>n

(n 
  10>s 
  n+0 n>@ (@>t) (t>s) s<" bottle" n-1 n>a? (a "s">s 0>a?) s<" of beer on the wall" 10>s
  n+0 n>@ (@>t) (t>s) s<" bottle" n-1 n>a? (a "s">s 0>a?) s<" of beer" 
  n-1 10>s<"Take one down and pass it around" 10>s  
  n+0 n>@ (@>t) (t>s) s<" bottle" n-1 n>a? (a "s">s 0>a?) s<" of beer on the wall" 10>s  
  n?
)

(s>o)