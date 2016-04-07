(make-base-namespace)
;(define ns (make-base-namespace))
(eval '(displayln "Eval works!"))

(define ns2 (make-base-namespace))
(eval '(displayln "Second eval also works!"))
