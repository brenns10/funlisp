(assert (equal? '(getattr a 'b) 'a.b))
(assert (equal? '(getattr (getattr one 'two) 'three) 'one.two.three))

(import example)

(assert (equal? (getattr example 'foo) "bar"))
(assert (equal? example.foo "bar"))

; OUTPUT(0)
