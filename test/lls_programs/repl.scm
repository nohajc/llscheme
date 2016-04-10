(define ns (make-base-namespace))

(define (repl)
  (define expr (read))
  (if (eof-object? expr)
	 null
	 (let ()
		(displayln (eval expr ns))
		(repl))))

(repl)
