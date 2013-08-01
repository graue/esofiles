#Computes the digital root of input number
#By Jannis Harder

#input
1>j
0>n
(i>s-10
  s?
  (s-22 s?
  (s-16
  j+0
  s+0
  j>t
  0>u
  (t-1 u+s+0 t?)
  n+u
  10>t
  0>u
  j+0
  (t-1 u+j+0 t?)
  0>j?
  u>j
  0>s?))
)
#calculate digital root
0>r n?
(n-1 t<n<0 n? t>n
  r+1 t<r<0 n? t>r
  r-10
  r>t?
  0>z
  (t<0 z? t?)
  (z? 1>r)
  0>t?
n?)
#output
r>@ o<10 (@>o)
