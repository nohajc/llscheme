(define ns (make-base-namespace))

(eval '(displayln "Eval works!") ns)
;(define ns2 (make-base-namespace))
(eval '(displayln "Second eval also works!") ns)

(eval '(define gstr "Global string is saved in the namespace.") ns)
(eval '(displayln gstr) ns)

(eval '(define sum (+ 3 4 5 6 7)) ns)
(eval '(displayln sum) ns)

(eval '(define (func x y z) (- (+ x y) z)) ns)
(eval '(displayln (func 7 8 3)) ns)

(eval '(let () (define lst '(2 3 4 5)) (displayln lst)) ns)
;(let () (define lst '(2 3 4 5)) (displayln lst))
