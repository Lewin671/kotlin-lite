# Parser Grammar (EBNF)

This document defines the formal grammar for the **kotlin-lite** language subset.

## 1. Complete Grammar

```ebnf
(* Top-Level Structure *)
KotlinFile       = { TopLevelObject } ;
TopLevelObject   = FunctionDecl ;

FunctionDecl     = "fun" Identifier "(" [ ParameterList ] ")" [ ":" Type ] Block ;
ParameterList    = Parameter { "," Parameter } ;
Parameter        = Identifier ":" Type ;

Block            = "{" { Statement } "}" ;

(* Statements *)
Statement        = VariableDecl
                 | Assignment
                 | IfStatement
                 | WhileStatement
                 | ReturnStatement
                 | "break"
                 | "continue"
                 | ExpressionStatement
                 | Block ;

VariableDecl     = ("val" | "var") Identifier [ ":" Type ] "=" Expression ;
Assignment       = Identifier "=" Expression ;

IfStatement      = "if" "(" Expression ")" Statement [ "else" Statement ] ;
WhileStatement   = "while" "(" Expression ")" Statement ;
ReturnStatement  = "return" [ Expression ] ;

ExpressionStatement = Expression ;

(* Expressions *)
Expression       = LogicalOr ;

LogicalOr        = LogicalAnd { "||" LogicalAnd } ;
LogicalAnd       = Equality { "&&" Equality } ;

Equality         = Comparison { ("==" | "!=") Comparison } ;
Comparison       = Addition { ("<" | "<=" | ">" | ">=") Addition } ;

Addition         = Multiplication { ("+" | "-") Multiplication } ;
Multiplication   = Unary { ("*" | "/" | "%") Unary } ;

Unary            = ("!" | "-") Unary 
                 | Primary ;

Primary          = Identifier [ "(" [ ArgumentList ] ")" ]
                 | IntegerLiteral
                 | FloatLiteral
                 | StringLiteral
                 | BooleanLiteral
                 | "(" Expression ")" ;

ArgumentList     = Expression { "," Expression } ;

(* Types and Literals *)
Type             = "Int" | "Boolean" | "Unit" | "Float" | "String" ;

Identifier       = ? Letter (Letter | Digit)* ? ;
IntegerLiteral   = ? Digit+ ? ;
FloatLiteral     = ? Digit+ "." Digit+ ? ;
StringLiteral    = ? '"' [^"]* '"' ? ;
BooleanLiteral   = "true" | "false" ;
```

## 2. Precedence Table

| Precedence | Operator | Description | Associativity |
| :--- | :--- | :--- | :--- |
| 1 (Lowest) | `||` | Logical OR | Left |
| 2 | `&&` | Logical AND | Left |
| 3 | `==`, `!=` | Equality | Left |
| 4 | `<`, `<=`, `>`, `>=` | Relational | Left |
| 5 | `+`, `-` | Additive | Left |
| 6 | `*`, `/`, `%` | Multiplicative | Left |
| 7 | `!`, `-` (unary) | Unary prefix | Right |
| 8 (Highest) | `()`, `f()` | Primary / Call | - |

## 3. Implementation Notes

1. **Top-Level Scope**: Following standard Kotlin (non-script) rules, the top-level scope only permits function declarations (`FunctionDecl`). All executable code, variable declarations, and logic must be contained within a function body.
2. **Assignment**: In Kotlin, assignments are statements and do not return values.
3. **Types**: While `Float` and `String` are recognized by the grammar, the current IR backend (Milestone 1-5) focuses on `Int` and `Boolean`. Using other types will trigger a semantic error during type checking.
4. **Built-in Functions**: `print_i32` and `print_bool` are syntactically treated as standard function calls. They are resolved during the semantic analysis phase.
