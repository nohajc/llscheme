(define (newline)
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

(let ((i (+ 4 5)))
  (define (noclos)
	 (display i))
  (noclos)
  (newline))

(foo (+ 2 3))
(newline)
(bar)
(newline)
