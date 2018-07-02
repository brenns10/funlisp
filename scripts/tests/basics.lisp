(assert (equal? (car '(1 2 3 4)) 1))
(assert (equal? (cdr '(1 2 3 4)) '(2 3 4)))

(assert-error 'LE_TYPE
              (car 5))
(assert-error 'LE_TYPE
              (car "blah"))
(assert-error 'LE_2FEW
              (car))
(assert-error 'LE_2MANY
              (car '(1) 5))
(assert-error 'LE_VALUE
              (car '()))

(assert-error 'LE_TYPE
              (cdr 5))
(assert-error 'LE_TYPE
              (cdr "blah"))
(assert-error 'LE_2FEW
              (cdr))
(assert-error 'LE_2MANY
              (cdr '(1) 5))
(assert-error 'LE_VALUE
              (cdr '()))

(assert-error 'LE_2FEW (cons 'a))
(assert-error 'LE_2MANY (cons 'a 'b 'c))


(assert (equal? (eval '(+ 1 1)) (+ 1 1)))
(assert (equal? (cons 'a '(b c d)) '(a b c d)))
(assert (equal? (cons 'a 'b) '(a . b)))
; OUTPUT(0)
