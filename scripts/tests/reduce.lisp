; correct usages
(assert (equal?
          (reduce + '(1 2 3 4))
          10))
(assert (equal?
          (reduce + 1 '(2))
          3))

; errors
(assert-error 'LE_2FEW
              (reduce +))
(assert-error 'LE_2MANY
              (reduce 'what 'an '(arg) 'party))
(assert-error 'LE_VALUE ; TODO this is a silly error
              (reduce + '(1)))
(assert-error 'LE_VALUE ; TODO this one too
              (reduce + 2 '()))
(assert-error 'LE_TYPE
              (reduce + "am i a list? no"))
(assert-error 'LE_TYPE
              (reduce + 'start-here 'but-im-not-a-list))

; OUTPUT(0)
