(define a 1)
(displayln a)

(define a 2)
(displayln a)

(define (foo a) (not a))
(displayln (foo #t))

(define (foo a) a)
(displayln (foo #t))
