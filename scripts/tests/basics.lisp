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

(assert-error 'LE_TYPE (define 5 5))

(assert (equal? (cond (0 "hello") (1 "there") (0 "testing")) "there"))
(assert (equal? (cond (1 "hello") (0 "there") (0 "testing")) "hello"))
(assert (equal? (cond (0 "hello") (0 "there") (1 "testing")) "testing"))
(assert (null? (cond (0 'nothing) (0 'to) (0 'eval) (0 'to) (0 'here))))

(assert-error 'LE_SYNTAX (cond))
(assert-error 'LE_SYNTAX (cond ()))
(assert-error 'LE_SYNTAX (cond (a b c)))
(assert-error 'LE_SYNTAX (cond 1))

; OUTPUT(0)
