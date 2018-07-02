; one argument function
(assert (equal?
          (map print '("just" "a" "list"))
          '(() () ())))
; two argument function
(assert (equal?
          (map + '(1 2 3) '(3 2 1))
          '(4 4 4)))

; argument errors:
(assert-error 'LE_2FEW
              (map +))

(assert-error 'LE_2MANY
              (map == '(4 6) '(2 2) '(? ?)))

(assert-error 'LE_VALUE
              (map == '(4 6) '(2 . 2)))

(assert-error 'LE_SYNTAX
              (map . ==))

(assert-error 'LE_SYNTAX
              (map == . 5))

; OUTPUT(0)
; just
; a
; list
