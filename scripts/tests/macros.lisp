; Create the "defun" and "defmacro" constructs and use them
(define defun (macro (name args code) `(define ,name (lambda ,args ,code))))
(define defmacro (macro (name args code) `(define ,name (macro ,args ,code))))
(defun add5 (x) (+ 5 x))
(assert (= (add5 5) 10))

; Test that the code from macros is evaluated in the correct scope
(defmacro setvalue (name value) `(define ,name ,value))
(define test 3)
(setvalue test 5)
(assert (= test 5))

; OUTPUT(0)
