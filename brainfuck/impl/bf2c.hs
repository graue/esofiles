{-
-- Brainfuck to C compiler
--
-- License: GPL
--
-- Author: Bertram Felgenhauer (bf3@inf.tu-dresden.de)
--
-- history:
-- 2002-10-(6-10) creation
-- 2002-10-22     fixed a small bug which caused assignments (Set *) get
--                moved through Reads instead of just being dropped
-}

import List
------------------------------------------------------------------------------
-- misc utility
------------------------------------------------------------------------------
{-
import PrelIOBase (unsafePerformIO)
debug :: String -> a -> a
debug s x = case unsafePerformIO ( putStrLn s ) of () -> x
-}
------------------------------------------------------------------------------
-- data types
------------------------------------------------------------------------------
data Term = Const Int           -- Int
          | Var Int             -- data[p+Int]
          | Sum [Term]          -- (Term+Term+...+Term)
          | Mul [Term]          -- (Term*Term*...*Term)
          deriving Show

data Prog = Loop Int [Prog] Int -- while (data[p+Int]) { [Prog]; p+=Int; }
          | If   Int [Prog] Int -- if    (data[p+Int]) { [Prog]; p+=Int; }
          | Read Int            -- data[p+Int] = getchar();
          | Write Int           -- putchar(data[p+Int]); fflush(stdout);
          | Set Int Term        -- data[p+Int] = Term;
          | Add Int Term        -- data[p+Int] += Term;
          | Move Int            -- p += Int;
          | Mark Prog           -- Prog
          deriving Show

------------------------------------------------------------------------------
-- some constants
------------------------------------------------------------------------------
m1 :: Int
m1 = -1

io     :: Int
io     = -2147483646  -- special variable identifier for io
allvar :: Int
allvar = -2147483647  -- special variable identifier for all variables

------------------------------------------------------------------------------
-- parsing
------------------------------------------------------------------------------
parse :: String -> ([Prog], String)
parse ""      = error "Too many '[' or missing ']'"
parse ('+':s) = Add 0 (Const  1) `ins` parse s
parse ('-':s) = Add 0 (Const m1) `ins` parse s
parse ('<':s) = Move m1 `ins` parse s
parse ('>':s) = Move  1 `ins` parse s
parse ('.':s) = Write 0 `ins` parse s
parse (',':s) = Read  0 `ins` parse s
parse ('[':s) = let (p, s') = parse s in Loop 0 p 0 `ins` parse s'
parse (']':s) = ([], s)
parse ( _ :s) = parse s

ins :: a -> ([a], b) -> ([a], b)
ins p (ps, s) = (p:ps, s)

------------------------------------------------------------------------------
-- remove Move-s
------------------------------------------------------------------------------
-- the idea here is to combine Move-s along the program by pushing
-- them to the end, pushing them through other operations by updating
-- these operation's offset; the final move offset is then incorporated
-- into the surrounding loop, or, in case of the main program, dropped.
norm_move :: [Prog] -> [Prog]
norm_move p = fst (nm p 0 0) where
    nm :: [Prog] -> Int -> Int -> ([Prog], Int)
    nm []               _ e = ([], e)
    nm (Move  s     :p) d e = nm p (d+s) (e+s)
    nm (Read  n     :p) d e = Read  (n+d)            `ins` nm p d e
    nm (Write n     :p) d e = Write (n+d)            `ins` nm p d e
    nm (Loop  n pp s:p) d e = let (pp', s') = nm pp d 0 in
                              Loop  (n+d) pp' (s+s') `ins` nm p d e
    nm (Set   n t   :p) d e = Set   (n+d) (mt d t)   `ins` nm p d e
    nm (Add   n t   :p) d e = Add   (n+d) (mt d t)   `ins` nm p d e
    nm p                _ _ = error ("nm " ++ show p)
    mt :: Int -> Term -> Term
    mt _ (Const i) = Const i
    mt d (Var i)   = Var (i+d)
    mt d (Sum rs)  = Sum (map (mt d) rs)
    mt d (Mul rs)  = Mul (map (mt d) rs)

------------------------------------------------------------------------------
-- utility functions for sorted lists
------------------------------------------------------------------------------
{-
uniqBy :: (a -> a -> Bool) -> [a] -> [a]
uniqBy _  []      = []
uniqBy _  [a]     = [a]
uniqBy eq (a:b:l) = if eq a b then uniqBy eq (b:l) else a:uniqBy eq (b:l)

uniq :: Eq a => [a] -> [a]
uniq = uniqBy (==)
-}

-- a merge function which drops duplicated entries
umergeBy ::  (a -> a -> Ordering) -> [a] -> [a] -> [a]
umergeBy _   al     []     = al
umergeBy _   []     bl     = bl
umergeBy cmp (a:al) (b:bl) = case cmp a b of
                       LT -> a:umergeBy cmp al (b:bl)
                       EQ ->   umergeBy cmp al (b:bl)
                       GT -> b:umergeBy cmp (a:al) bl

umerge :: Ord a => [a] -> [a] -> [a]
umerge = umergeBy compare

-- tests whether two sorted lists have common entries
comm :: Ord a => [a] -> [a] -> Bool
comm _      []     = False
comm []     _      = False
comm (a:al) (b:bl) = case compare a b of
                     LT -> comm al (b:bl)
                     EQ -> True
                     GT -> comm (a:al) bl

-- specialized version of comm, which handles the 'allvar' variable
collide :: [Int] -> [Int] -> Bool
collide _      []     = False
collide []     _      = False
collide (a:al) (b:bl) = if a==allvar || b==allvar then True
                                                  else comm (a:al) (b:bl)

------------------------------------------------------------------------------
-- merging, optimization
------------------------------------------------------------------------------
free_vars :: Term -> [Int]
free_vars (Const _) = []
free_vars (Var i)   = [i]
free_vars (Sum rs)  = foldr umerge [] (map free_vars rs)
free_vars (Mul rs)  = foldr umerge [] (map free_vars rs)

-- returns the variables an operation depends on
used_vars :: Prog -> [Int]
used_vars (Read  _)     = [io]
used_vars (Write i)     = [io, i]
used_vars (Loop  i p 0) = foldr umerge [i] (map used_vars p)
used_vars (Loop  i p _) = [allvar]
used_vars (If    i p 0) = foldr umerge [i] (map used_vars p)
used_vars (If    i p _) = [allvar]
used_vars (Set   _ t)   = free_vars t
used_vars (Add   i t)   = umerge [i] (free_vars t)
used_vars (Mark  p)     = used_vars p
used_vars p             = error ("used_vars " ++ show p)

-- returns the variables changed by an operation
set_vars :: Prog -> [Int]
set_vars (Read  i)     = [io, i]
set_vars (Write _)     = [io]
set_vars (Loop  _ p 0) = foldr umerge [] (map set_vars p)
set_vars (Loop  _ p _) = [allvar]
set_vars (If    _ p 0) = foldr umerge [] (map set_vars p)
set_vars (If    _ p _) = [allvar]
set_vars (Set   i _)   = [i]
set_vars (Add   i _)   = [i]
set_vars (Mark  p)     = set_vars p
set_vars p             = error ("set_vars " ++ show p)

-- takes two terms, returns a term corresponding to their sum;
-- does some simplifications
add_term :: Term -> Term -> Term
add_term (Const x)         b                 = add_term_const x b
add_term a                 (Const x)         = add_term_const x a
add_term (Sum (Const x:a)) (Sum b)           = add_term_const x (Sum (b++a))
add_term (Sum a)           (Sum (Const x:b)) = add_term_const x (Sum (a++b))
add_term (Sum a)           (Sum b)           = Sum (a++b)
add_term (Sum a)           b                 = Sum (a++[b])
add_term a                 (Sum b)           = Sum (b++[a])
add_term a                 b                 = Sum [a,b]

add_term_const :: Int -> Term -> Term
add_term_const 0 t                  = t
add_term_const b (Const a)          = Const (a+b)
add_term_const b (Sum (Const a:as)) = add_term_const (a+b) (Sum as)
add_term_const b (Sum as)           = Sum   (Const b:as)
add_term_const b t                  = Sum   [Const b, t]

-- likewise for the product of two terms
mul_term :: Term -> Term -> Term
mul_term (Const x)         b                 = mul_term_const x b
mul_term a                 (Const x)         = mul_term_const x a
mul_term (Mul (Const x:a)) (Mul b)           = mul_term_const x (Mul (b++a))
mul_term (Mul a)           (Mul (Const x:b)) = mul_term_const x (Mul (a++b))
mul_term (Mul a)           (Mul b)           = Mul (a++b)
mul_term (Mul a)           b                 = Mul (a++[b])
mul_term a                 (Mul b)           = Mul (b++[a])
mul_term a                 b                 = Mul [a,b]

mul_term_const :: Int -> Term -> Term
mul_term_const 0 _                  = Const  0
mul_term_const 1 t                  = t
mul_term_const b (Const a)          = Const (a*b)
mul_term_const b (Mul (Const a:as)) = mul_term_const (a*b) (Mul as)
mul_term_const b (Mul as)           = Mul   (Const b:as)
mul_term_const b (Sum as)           = Sum   (map (mul_term_const b) as)
mul_term_const b t                  = Mul   [Const b, t]

-- the dependency checking optimization process ...
combine :: [Prog] -> [Prog]
combine = unmark.docombine.reccomb

-- remove Mark-s left in by docombine.
unmark :: [Prog] -> [Prog]
unmark []          = []
unmark (Mark p:ps) = p:unmark ps
unmark (p     :ps) = p:unmark ps

-- first call combine in all the Loops in the program, and call
-- 'flatten' to try to unroll the loops.
reccomb :: [Prog] -> [Prog]
reccomb []                = []
reccomb ((Loop i p j):ps) = flatten (Loop i (combine p) j) ++ reccomb ps
reccomb ((If   i p j):ps) = flatten (If   i (combine p) j) ++ reccomb ps
reccomb (p           :ps) = p:reccomb ps

-- find suitable candidates for combining them with following operations
-- which change the same variable
-- Mark-ed entries will be skipped; this gives push_opc a way to mark
-- already processed entries.
docombine :: [Prog] -> [Prog]
docombine []               = []
docombine (m@(Mark  _):ps) = m:docombine ps
docombine (t@(Set i _):ps) =
    push_opc i t (used_vars t) (set_vars t) [] [] [] [] ps
docombine (t@(Add i _):ps) =
    push_opc i t (used_vars t) (set_vars t) [] [] [] [] ps
docombine (t@(Read  _):ps) = t:docombine ps
docombine (t@(Write _):ps) = t:docombine ps

-- push_opc v x xus xch p pus pch m ps
-- push an opcode, x, which changes the variabe v,
-- uses variables xus, changes variables xch, where
-- p are opcodes 'pushed ahead' which depend on x or other opcodes
-- in p, pus and pch are the used and changed variables of all of p,
-- and ps is the code which the opcode (and p) is going to be pushed
-- through ... if x meets another operation which depends on x,
-- an attempt is made to combine the two.
push_opc :: Int -> Prog -> [Int] -> [Int] -> [Prog] -> [Int] -> [Int] ->
            [Prog] -> [Prog] -> [Prog]
push_opc v x xus xch p pus pch m [] =
    docombine (reverse m ++ Mark x:reverse p)
push_opc v x xus xch p pus pch m (y:ps) =
    let rec = push_opc v x xus xch (y:p) (umerge pus yus) (umerge pch ych) m ps
        flr = docombine (reverse m ++ Mark x:reverse p ++ y:ps)
        yus = used_vars y; ych = set_vars y in
    if collide yus pch || collide pus ych
    then rec
    else case y of
    Set w t | w==v -> if collide [v] (free_vars t) then flr
        else push_opc v y yus ych p pus pch m ps
    Add w t | w==v -> if collide [v] (free_vars t) then flr
        else case x of
        Add _ s -> push_opc v (Add v (add_term s t))
                            (umerge yus xus) (xch) p pus pch m ps
        Set _ s -> push_opc v (Set v (add_term s t))
                            (umerge yus xus) (xch) p pus pch m ps
    Read w  | w==v -> docombine (reverse m ++ reverse p ++ y:ps)
    _              -> if collide yus xch || collide ych xus then rec
        else push_opc v x xus xch p pus pch (y:m) ps

-- optimize loops which don't change the pointer and just increment
-- or decrement a designated counter variable by one in each
-- operation
flatten :: Prog -> [Prog]
flatten x = case x of
    Loop i p 0 -> let (si, ind, lop, _) = find_indep i [io] p in
        case si of
        Just (Set _ t) -> case t of
            Const 0 -> [Mark (If i p 0)]
            Const _ -> [Mark (Loop i (Set i t:lop) 0)]
            _       -> [Mark x]
        Just (Add _ t) -> case t of
            Const (-1) -> let (ind', onc, lop') = times (Var i) ind in
                          let lp = let lop'' = lop ++ lop' in case lop'' of
                                   [] -> [Set  i (Const 0)]
                                   _  -> [Mark (Loop i (Add i t:lop'') 0)] in
                          ind' ++ case onc of
                              [] -> lp
                              _  -> [Mark (If i (onc ++ lp) 0)]
            Const 0    -> [Mark (If i [Loop i (Add i t:lop) 0] 0)]
            Const 1    -> let (ind', onc, lop') = times (Mul [Const m1, Var i]) ind in
                          let lp = let lop'' = lop ++ lop' in case lop'' of
                                   [] -> [Set  i (Const 0)]
                                   _  -> [Mark (Loop i (Add i t:lop'') 0)] in
                          ind' ++ case onc of
                              [] -> lp
                              _  -> [Mark (If i (onc ++ lp) 0)]
            _          -> [Mark x]
        _              -> [Mark x]
    If   i p 0 -> [Mark x]
    _          -> [Mark x]

-- times t p (t is the number of times the loop is executed)
-- collect from p operations which
-- a) can be streamlined, that is, the loop count can be incorporated into
--    the operation (example: Add i 1 -> Add i t)
-- b) have to be executed once *if* the loop is executed at all
--    (most of 'Set' operations)
-- c) have to be executed inside the normal loop because transforming
--    them into a) or b) fails for some reason.
times :: Term -> [Prog] -> ([Prog], [Prog], [Prog])
times t []     = ([], [], [])
times t (p:ps) = let (m, o, l) = times t ps in case p of
    Set i t' | notElem i (free_vars t') -> (m, p:o, l)
    Add i t' | notElem i (free_vars t') -> (Add i (mul_term t t'):m, o, l)
    _                                   -> (m, o, p:l)

-- find_indep i us ps
-- find suitable candidates for 'times' above, that is, operations which
-- only depend upon themselves in consecutive loop operations.
find_indep :: Int -> [Int] -> [Prog] -> (Maybe Prog, [Prog], [Prog], [Int])
find_indep i _  []     = (Nothing, [], [], [io])
find_indep i us (p:ps) =
    let pus = umerge (used_vars p) (set_vars p) in
    let (si, ind, lop, us') = find_indep i (umerge pus us) ps in
    if collide pus us || collide pus us'
        then (si, ind, p:lop, umerge pus us')
        else case p of
            Set i' _ | i==i' -> (Just p, ind, lop, umerge pus us')
            Add i' _ | i==i' -> (Just p, ind, lop, umerge pus us')
            _                -> (si,   p:ind, lop, umerge pus us')

optimize :: [Prog] -> [Prog]
optimize = combine.norm_move

------------------------------------------------------------------------------
-- output
------------------------------------------------------------------------------
-- indent by two spaces
isp :: String
isp = "  "

indent :: [String] -> [String]
indent = map (isp ++)

int :: String -> [String] -> String
int i [s]   = s
int i (t:s) = t ++ i ++ int i s

show_var :: Int -> String
show_var i = if i<0  then "data[p-" ++ show (-i) ++ "]"
        else if i==0 then "data[p]"
        else              "data[p+" ++ show i ++ "]"

show_term :: Term -> String
show_term (Var i)   = show_var i
show_term (Sum l)   = "(" ++ int " + " (map show_term l) ++ ")"
show_term (Mul l)   = "(" ++ int " * " (map show_term l) ++ ")"
show_term (Const i) = show i

show_progs :: [Prog] -> [String]
show_progs []     = []
show_progs (p:ps) = (case p of
    Mark p     -> show_progs [p]
    Loop i p d -> ["for ( ; " ++ show_var i ++ "; "
                   ++ (if d==0 then "" else "p += " ++ show d) ++ ") {"]
                  ++ indent (show_progs p) ++ ["}"]
    If   i p d -> ["if (" ++ show_var i ++ ") {"]
                   ++ indent (show_progs p) ++
                  if d==0 then ["}"]
                          else [isp ++ "p += " ++ show d ++ ";", "}"]
    Read  i    -> [show_var i ++ " = getchar();"]
    Write i    -> ["putchar(" ++ show_var i ++ "); fflush(stdout);"]
    Set   i t  -> [show_var i ++ " = " ++ show_term t ++ ";"]
    Add   i t  -> [show_var i ++ " += " ++ show_term t ++ ";"])
    ++ show_progs ps

intro :: [String]
intro = [
  "#include <stdio.h>",
  "",
  "signed int p;",
  "signed int data[65536];",
  "",
  "int main(void)",
  "{"]

show_program :: [Prog] -> [String]
show_program p = intro ++ indent (show_progs p) ++ ["}"]

compile :: String -> String
compile s = let (p, s') = parse (s ++ "]") in
            if s' /= "" then error "Missing '[' or too many ']'"
                        else (unlines.show_program.optimize) p

------------------------------------------------------------------------------
-- main
------------------------------------------------------------------------------
-- act as a simple filter; read brainfuck code from stdin and
-- write C code to stdout.
main = interact compile
