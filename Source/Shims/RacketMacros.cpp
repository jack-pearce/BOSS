#include <string>
/**
 * This is some racket code that establishes a convenient interface to BOSS --
 * right now I am just doing it using a raw string literal (maybe we'll use our expression builder
 * at some point)
 */
std::string getRacketMacroShims() {
  // NOLINTNEXTLINE
  return
      R"(
(require racket/match)
(define (convert-to-boss-expression x)
  (match x
         [(list head arguments ...) (new-Expression (new-ComplexExpression head (map convert-to-boss-expression arguments)))]
         [(and i (? integer?)) (new-Expression i)]
         [(and s (? string?)) (new-Expression s)]
         [(and s (? symbol?)) (new-Expression (new-Symbol (symbol->string s)))]
         [_ 'unknown]
         )
  )
(define-syntax-rule (InsertInto args ...) ((lambda () (evaluate (convert-to-boss-expression '(InsertInto args ...))) (void))))
(define-syntax (Plus syntax-object) (syntax-case syntax-object () ((_ args ...) #'(evaluate (convert-to-boss-expression '(Plus args ...))))))
(define-syntax (Where syntax-object) (syntax-case syntax-object () ((_ args ...) #'(evaluate (convert-to-boss-expression '(Where args ...))))))
(define-syntax (Greater syntax-object) (syntax-case syntax-object () ((_ args ...) #'(evaluate (convert-to-boss-expression '(Greater args ...))))))
(define-syntax (CreateTable syntax-object) (syntax-case syntax-object () ((_ args ...) #'(evaluate (convert-to-boss-expression '(CreateTable args ...))))))
(define-syntax (GroupBy syntax-object) (syntax-case syntax-object () ((_ args ...) #'(evaluate (convert-to-boss-expression '(GroupBy args ...))))))
(define-syntax (Select syntax-object) (syntax-case syntax-object () ((_ args ...) #'(evaluate (convert-to-boss-expression '(Select args ...))))))
(define-syntax (Project syntax-object) (syntax-case syntax-object () ((_ args ...) #'(evaluate (convert-to-boss-expression '(Project args ...))))))
)";
}
