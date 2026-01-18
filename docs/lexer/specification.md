# Lexical Specification

This document defines the lexical structure of the **kotlin-lite** language.

## 1. Character Classes

| Class | Definition |
| :--- | :--- |
| `Digit` | `[0-9]` |
| `Letter` | `[a-zA-Z_]` |
| `Whitespace` | `[ \t\r]` |
| `Newline` | `\n` |

## 2. Tokens

### 2.1 Keywords

| Category | Keywords |
| :--- | :--- |
| **Supported** | `fun`, `val`, `var`, `if`, `else`, `while`, `return`, `break`, `continue`, `true`, `false`, `null` |
| **Reserved** | `package`, `import`, `class`, `interface`, `when`, `for`, `as`, `is`, `this`, `super`, `in` |

*Reserved keywords are recognized by the lexer to prevent them from being used as identifiers, but they are not currently used in the grammar.*

### 2.2 Literals

| Token Type | Example |
| :--- | :--- |
| `IDENTIFIER` | `userName`, `result_1` |
| `INTEGER` | `123` |
| `FLOAT` | `3.14` |
| `STRING` | `"Hello"` |

### 2.3 Operators and Delimiters
`+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`, `!`, `(`, `)`, `{`, `}`, `,`, `.`, `:`, `;`, `->`

## 3. Special Rules

### 3.1 Semicolon Insertion
A newline acts as a statement terminator if the previous token can end a statement (Identifier, Literal, `)`, `}`, `break`, `continue`, `return`) and the next token can start one.

### 3.2 Comments
- `// Line comment`
- `/* Block comment (nested) */`