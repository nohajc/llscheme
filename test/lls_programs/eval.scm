(make-base-namespace)
;(define ns (make-base-namespace))
(eval '(displayln "Eval works!"))

(define ns2 (make-base-namespace))
(eval '(displayln "Second eval also works!"))

(eval '(let () (define lst '(2 3 4 5)) (displayln lst)))
;(let () (define lst '(2 3 4 5)) (displayln lst))
