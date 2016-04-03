(define (newline) ; Comment test
  (display "\n"))

(define (foo i)
  (define (clos)
    (display i))
  (clos))

(define (bar)
  (let ((i (+ 3 4)))
	 (define (clos2)
		(display i))
	 (clos2)))

; Comment ; inside a comment

(let ((i (+ 4 5)))
  (define (noclos)
	 (display i))
  (noclos)
  (newline))

(define (clos3 a b)
  (lambda ()
	 (lambda (x)
		(+ a b x))))

(foo (+ 2 3))
(newline)
(bar)
(newline)

(display (((clos3 1 2)) 4))
(newline)

(define (simple-apply fn a b)
  (fn a b))

(display (simple-apply + 7 2))
(newline)
(display (simple-apply - 8 3))
(newline)

;(display (((clos3 1 2) 3) 4))
