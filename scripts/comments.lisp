; I'm a line comment. I'm the only type of comment allowed in funlisp. I can
; occur on a blank line or at the end of a line already containing code.

(+ 5;this is fine
   5)

(define main;this is also fine
  (;still ok
   lambda (x
            ;also cool
            )
   (print "hello world";you guessed it: fine
          )))

; comments ending the file shouldn't cause any problems either
