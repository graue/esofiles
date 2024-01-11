+++++++[>+++++++<-]>.+..-..[-] ; Print out "12211"
+>>>>++>>>>++>>>>+>+>>>+>>+<<<<<<<<<<<<<<<<<<< ; Store 1 2 2 1 1
                                               ;            s^  f^
+[ ; Infinite loop
    >>-[+>>>>-]+ ; Go to the slow pointer
    <[
        >>-[+>>>>-]+ ; Go to the fast pointer
        >>[>>>>]+++ ; Set top value to 3
        <<-[+<<<<-]+ ; Go back to the fast pointer
        <<[>>>+>[>>>>]<<<<-<<-[+<<<<-]+<<-] ; Set top value to 3 minus fast pointer value
        >>>[<<<+>>>-] ; Restore fast pointer value
        <<<[>>>>]<<<++++++[<++++++++>-]<. ; Output top value as character
        >++++++[<-------->-] ; Restore to number
        -[+<<<<-]+<- ; Decrement slow pointer value
    ]
    >>>[<<<+>>>-] ; Restore slow pointer value
    <-[+>>>>-]+ ; Go to the fast pointer
    [>>[>>>>]<<+<<<<-[+<<<<-]] ; Move fast pointer to top
    <-[+<<<<-]+[>>>>+<<<<-] ; Move slow pointer to the next
    >>-[+<<<<-]+ ; Move back to infinite loop flag
]
