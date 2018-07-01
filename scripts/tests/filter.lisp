(define filter (lambda (f l)
    (if (null? l)
      '()
      (if (f (car l))
        (cons (car l) (filter f (cdr l)))
        (filter f (cdr l))))))

(define main
  (lambda (args)
    (print (filter (lambda (x) (< x 5)) '(1 2 3 4 5 6 7 8 9 10)))))

; OUTPUT(0)
; (1 2 3 4 )
