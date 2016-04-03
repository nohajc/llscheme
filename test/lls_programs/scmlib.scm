; Library functions implementation

(define (cadr l) (car (cdr l)))
(define (cddr l) (cdr (cdr l)))
(define (zero? n) (= n 0))

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

(define (displayln obj)
  (display obj)
  (display "\n"))

; Test the functions
(define lst (quote (1 2 3 4 5)))
(displayln (map (lambda (x) (+ x x)) lst))
