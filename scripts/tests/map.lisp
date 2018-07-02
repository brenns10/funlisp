; correct usage:
(assert (equal?
          (map + '(1 2 3) '(3 2 1))
          '(4 4 4)))

; argument errors:
(assert-error 'LE_2FEW
              (map +))

; OUTPUT(0)
