; Library functions implementation

(define (identity x) x)

(define (not obj) (if obj #f #t))
(define (zero? n) (= n 0))

(define (cadr l) (car (cdr l)))
(define (cddr l) (cdr (cdr l)))

(define (list-ref lst idx)
  (if (null? lst)
	 null
	 (if (zero? idx)
	   (car lst)
	   (list-ref (cdr lst) (- idx 1)))))

(define (>= a b) (or (> a b) (= a b)))
(define (<= a b) (not (> a b)))
(define (< a b) (not (>= a b)))

(define (append a b)
  (if (null? a) b (cons (car a) (append (cdr a) b))))

(define (map fn lst)
  (if (null? lst)
    null
    (cons (fn (car lst)) (map fn (cdr lst)))))

(define (filter pred lst)
  (if (null? lst)
    null
    (if (pred (car lst))
      (cons (car lst) (filter pred (cdr lst)))
      (filter pred (cdr lst)))))

(define (zip a b)
  (if (or (null? a) (null? b))
    null
    (cons (list (car a) (car b)) (zip (cdr a) (cdr b)))))

(define (member v lst)
  (if (null? lst)
	 #f
	 (or (equal? (car lst) v)
		  (member v (cdr lst)))))

(define (uniq_r lst added)
  (if (null? lst)
	 null
	 (if (member (car lst) added)
		(uniq_r (cdr lst) added)
		(cons (car lst) (uniq_r (cdr lst) (cons (car lst) added))))))

(define (remove-duplicates lst)
  (uniq_r lst null))

(define (compose1 fn1 fn2)
  (lambda (x) (fn1 (fn2 x))))

(define (displayln obj)
  (display obj)
  (display "\n"))

(define (newline) (display "\n"))

; This is for debugging.
; We just want to see if the library was loaded.
;(display "Hello library!\n")
