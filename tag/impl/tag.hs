---
--- The Tag programming language
--- A pathological language based on a variable Post Tag machine.
--- Copyright 2007, Mark C. Chu-Carroll <markcc@gmail.com>
--- http://www.scienceblogs.com/goodmath
--- 
--- You are free to do whatever you want with this code so long as you include
--- the above copyright notice.
--- 
---
---

module Main where

import Text.ParserCombinators.Parsec
import qualified Text.ParserCombinators.Parsec.Token as P
import Text.ParserCombinators.Parsec.Language 
import System.Environment	
	
data Rule = Rule String Char [Sym] Integer Bool deriving Show
         -- Rule name char stringToAppend numberToDrop printDropped


data TagSystem = TagSystem [Rule] deriving Show
--                         rule

data Sym = SymChar Char | SymHalt | SymInput deriving (Eq,Show)


data TagSystemQueue = TSQ [Sym] deriving (Show)

peekFirst :: TagSystemQueue -> Maybe Sym
peekFirst (TSQ (v:vs)) = Just v
peekFirst (TSQ []) = Nothing

removeN :: Integer -> TagSystemQueue -> ([Sym],TagSystemQueue)
removeN _ t@(TSQ []) = ([], t)
removeN 1 (TSQ (v:vs)) = ([v], TSQ vs)
removeN n (TSQ (v:vs)) = 
	let (front, back) = removeN (n-1) (TSQ vs)
	in ((v:front), back)

appendCharsToQueue :: TagSystemQueue -> [Sym] -> TagSystemQueue
appendCharsToQueue (TSQ queue) syms = TSQ (queue ++ syms)

isEmpty :: TagSystemQueue -> Bool
isEmpty (TSQ (x:xs)) = False
isEmpty (TSQ []) = True


findRule ::  Sym -> [Rule] -> Maybe Rule
findRule sym@(SymChar c) (r@(Rule _ d _ _ _):rules) | c == d = Just r
                                    | otherwise = findRule sym rules
findRule c [] = Nothing
findRule SymHalt _ = Nothing

stepTagSystem :: TagSystem -> TagSystemQueue -> IO (Sym,TagSystemQueue)
stepTagSystem t@(TagSystem rules) input =
    if stringNotEmpty 
        then case rule of
            Just r -> runRule rule input
            Nothing -> if (firstSym == SymHalt)
	                      then return (SymHalt, input)
		                  else error "No matching rule"
        else return (firstSym,input)        where 
                        (stringNotEmpty,firstSym) = case input of
                                     (TSQ []) -> (False,SymHalt)
                                     (TSQ (s:rest)) -> (True,s)
                        rule = findRule firstSym rules

runRule :: (Maybe Rule) -> TagSystemQueue -> IO (Sym,TagSystemQueue)
runRule (Just (Rule name c syms num out)) queue =
    let firstSym = peekFirst queue
        (leader,queueFront) = removeN num queue
    in case firstSym of 
         Nothing -> return (SymHalt, queueFront)
         Just s ->  do { if out then putStr (queueToString (tail leader))
	                            else return ()
                       ; return (s, appendCharsToQueue queueFront syms)
                       }
runRule Nothing queue =
	return (SymHalt, queue)

runTagSystem :: TagSystem -> TagSystemQueue -> IO TagSystemQueue
runTagSystem tagSys queue =
    if (isEmpty queue) 
	    then return queue
        else do { (c,updatedQueue) <- stepTagSystem tagSys queue
                ; case c of
                    SymHalt -> return updatedQueue
                    SymChar _ -> runTagSystem tagSys updatedQueue
                }


queueToString :: [Sym] -> String
queueToString (sym:syms) =
        case sym of
           SymChar c -> (c:(queueToString syms))
           SymHalt -> ('#':(queueToString syms))
           SymInput -> ('?':(queueToString syms))
queueToString [] = []
            

lexer :: P.TokenParser ()

lexer = P.makeTokenParser (haskellDef
                           { P.reservedNames = ["Halt"] })

number = P.integer lexer
ident = P.identifier lexer
whitespace = P.whiteSpace lexer
symbol = P.symbol lexer
colon = symbol ":"
bang = symbol "!"
question = do { s <- symbol "?"
              ; return [SymInput]
              }

parens = P.parens lexer
halt = do { h <- symbol "#"
          ; return SymHalt
          }


stringChar = do { a <- noneOf ")#\n"
                ; return $ SymChar a
                }

stringElement = (stringChar <|> halt)

appendString =  (many stringElement) <|> question

ruleDecl = do { sym <- ident
              ; colon
              ; c <- alphaNum
              ; n <- number
              ; t <- option False (do{ b <- bang
                                     ; return True
                                     })
              ;  str <- parens appendString
              ; return $ Rule sym c str n t
              } 
           
tagProgram = do { rules <- many1 ruleDecl
                ; return $ TagSystem rules
                }


prun :: (Show a) => Parser a -> String -> IO a
prun p input 
	= case (parse p "" input) of 
		Left err -> do{ putStr "parse error at " 
		; print err 
		; error "Parse error"
		} 
		Right x -> return x

parseTagSystemString :: String -> IO  TagSystem
parseTagSystemString input =
    prun (do { whitespace
             ; x <- tagProgram
             ; eof
             ; return x}) input


parseAndRun :: String -> String -> IO String
parseAndRun program input =
    do 
        tagsys <- parseTagSystemString program
        str <- prun appendString input
        (TSQ end) <- runTagSystem tagsys (TSQ str)
        s <- return $ queueToString end
        putStrLn ""
        putStrLn "-------------------------"
        putStrLn s
        return s


main =
	do 
       (file:(input:[])) <- getArgs
       parsedTagsys <- parseFromFile (do { whitespace
                              ; x <- tagProgram
                              ; eof
                              ; return x }) file       
       case parsedTagsys of
          Right tagsys -> do 
             iQ <- prun appendString input
             (TSQ result) <- runTagSystem tagsys (TSQ iQ)
             putStrLn "------------------------"
             putStrLn (queueToString result)
          Left err -> do
            putStrLn ("Parse Error"++(show err))

		

