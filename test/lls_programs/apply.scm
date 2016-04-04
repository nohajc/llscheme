(define lst '(2 4 6 8))

(display (apply + lst))
(display "\n")

(apply display (cons (apply + lst) null))
(display "\n")

(define (foo0)
  (display "Called foo0")
  (display "\n"))

;(foo0)
(apply foo0 null)

(define (foo4 a b c d)
  (+ (- (+ a b) c) d))

(display (apply foo4 lst))
(display "\n")

(define (makeprinter obj)
  (lambda ()
	 (display obj)
    (display "\n")))

(apply (apply makeprinter (cons lst null)) null)

;(apply makeprinter (cons lst null))
((apply apply (cons makeprinter (cons (cons lst null) null))))

; Error: not enough arguments
;(display (apply foo4 (quote (1 2 3))))
