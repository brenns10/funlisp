(assert (equal? '(getattr a 'b) 'a.b))
(assert (equal? '(getattr (getattr one 'two) 'three) 'one.two.three))

(import os)

(import filter)

(assert (equal?
          (filter.filter (lambda (v) (< v 3)) '(1 2 3 4 5))
          '(1 2)))

; OUTPUT(0)
