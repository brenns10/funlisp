; quasiquote samples
(define x 4)
(assert (equal?
          `(1 2 3 ,x)
          '(1 2 3 4)))

(assert (equal?
          `(1 2 3 ,(+ 2 2))
          '(1 2 3 4)))

; errors occurring in quasiquote are errors in real life
(assert-error 'LE_VALUE
              `(blah (blah blah) ,(/ 1 0) (blah)))

(assert-error 'LE_2FEW (quasiquote))
(assert-error 'LE_2MANY (quasiquote a b))
(assert-error 'LE_2FEW (unquote))
(assert-error 'LE_2MANY (unquote a b))
(assert-error 'LE_2FEW (quote))
(assert-error 'LE_2MANY (quote a b))
; OUTPUT(0)
