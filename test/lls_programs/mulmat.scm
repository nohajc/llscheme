(require "lib/matrix")

;(define m '((1 2) (3 4)))
;(displayln (mul-mat m m))

(define (multiply-input)
  (let ((m1 (read)) (m2 (read)))
	 (if (eof-object? m1)
		null
		(let ()
		  (displayln (mul-mat m1 m2))
		  (multiply-input)))))

(multiply-input)
