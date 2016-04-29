(define row list-ref)

(define (col mat idx)
  (if (null? mat)
    null
	 (cons (list-ref (car mat) idx) (col (cdr mat) idx))))

(define (mul-elem row col)
  (foldl + 0 (map (lambda (pair) (* (car pair) (cadr pair))) (zip row col))))

(define (mul-row row mat2 j colnum)
  (if (= j colnum)
    null
	 (cons (mul-elem row (col mat2 j)) (mul-row row mat2 (+ j 1) colnum))))

(define (mul-rows mat1 mat2 colnum)
  (if (null? mat1)
    null
	 (cons (mul-row (row mat1 0) mat2 0 colnum) (mul-rows (cdr mat1) mat2 colnum))))

(define (mul-mat mat1 mat2)
  (mul-rows mat1 mat2 (length mat1)))

;(displayln (mul-mat '((1 2) (3 4)) '((5 6) (7 8))))
;(define m '((1 2 3) (4 5 6) (7 8 9)))
;(displayln (mul-mat m m))
