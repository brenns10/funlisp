; test that we can bind values
(let ((x 5)) (assert (equal? x 5)))

; test that we can bind many values
(let ((x 5) (y 5)) (assert (equal? 10 (+ x y))))

; test that we can execute multiple lines of code in let
(let ((x 5) (y 5))
  (assert (equal? 5 x))
  (assert (equal? 5 y)))

; test that we can refer to prior bindings
(let ((x 1)
      (y (+ 1 x)))
  (assert (equal? y 2)))

; test that we can even refer to future bindings later on :)
(let ((return2 (lambda () (+ 1 (return1))))
      (return1 (lambda () 1)))
  (assert (equal? (return2) 2))
  (assert (equal? (return1) 1)))

; it should even work fine without any bindings...
(assert (equal? (let () 5) 5))

; errors
(assert-error 'LE_2FEW
  (let))
(assert-error 'LE_2FEW
  (let ()))
(assert-error 'LE_TYPE
  (let (not-a-binding-list-haha) 5))
(assert-error 'LE_TYPE
  (let ((1 'that-is-not-a-symbol??)) 5))
(assert-error 'LE_2FEW
  (let ((i-am-too-few-to-be-a-binding)) 5))
(assert-error 'LE_2MANY
  (let ((name 'which-value 'should-i-choose?)) 5))

; OUTPUT(0)
