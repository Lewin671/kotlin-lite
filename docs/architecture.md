# Kotlin Compiler Architecture

## Overview

This document outlines the design and implementation roadmap for a Kotlin subset compiler. The compilation pipeline follows a linear transformation sequence:

**Source Code → AST → Semantic Analysis/Type Checking → Custom SSA-CFG IR → LLVM IR → Object Code → Runtime Linking → Executable**

---

## 1. Language Subset Specification

The compiler supports a strictly defined subset of Kotlin. All other language features must trigger explicit error reports.

### Supported Features

**Types:**
- `Int` (LLVM `i32`)
- `Boolean` (LLVM `i1`)
- `Unit` (LLVM `void`)

**Functions:**
- Top-level functions only (no nested functions or closures)

**Variables:**
- Local variables with `val` and `var` declarations

**Expressions:**
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical: `!`, `&&`, `||` (with short-circuit evaluation)

**Statements:**
- Conditional: `if`/`else`
- Loops: `while` with `break` and `continue`
- Control flow: `return`

**Built-in Functions:**
- `print_i32(x: Int)` - Print 32-bit integer
- `print_bool(b: Boolean)` - Print boolean value

---

## 2. Custom Intermediate Representation (IR)

The custom IR uses a **Static Single Assignment (SSA) form** combined with **Control Flow Graphs (CFG)**. Variables are represented as SSA values at the IR level, eliminating the need for `alloca`, `load`, and `store` instructions.

### 2.1 Structure

The IR is organized hierarchically:

```
Module
├── Function[]
    ├── BasicBlock[]
    │   ├── Instruction[]
    │   └── Terminator (required, one per block)
```

### 2.2 Type System

- `i32` - 32-bit signed integer
- `i1` - 1-bit boolean
- `void` - Unit type (no return value)

### 2.3 Instruction Set

**Value Instructions** (produce SSA values):

| Instruction | Description | Type |
|---|---|---|
| `const_i32(value)` | 32-bit integer constant | → `i32` |
| `const_i1(value)` | Boolean constant | → `i1` |
| `add`, `sub`, `mul` | Arithmetic operations | `i32, i32 → i32` |
| `sdiv`, `srem` | Signed division and remainder | `i32, i32 → i32` |
| `icmp(cond)` | Integer comparison (`eq`, `ne`, `slt`, `sle`, `sgt`, `sge`) | `i32, i32 → i1` |
| `not` | Logical negation | `i1 → i1` |
| `phi(type, [(pred, val), ...])` | Control flow join point | `→ type` |
| `call(fn, args...)` | Function call | `→ (i32 \| i1 \| void)` |

**Terminator Instructions** (control flow):

| Terminator | Description |
|---|---|
| `br(target)` | Unconditional branch to target block |
| `condbr(cond, thenBB, elseBB)` | Conditional branch |
| `ret([val])` | Return from function (optional value) |

**Design Principle:** The IR specification is intentionally minimal. Focus on core functionality and avoid adding advanced features prematurely.

---

## 3. AST to IR Lowering: Environment-Based SSA Construction

The core mechanism for SSA generation relies on maintaining an **environment** that tracks variable bindings, combined with **phi node insertion** at control flow merge points.

### 3.1 Environment Model

```
env : VariableName → SSAValue
```

The environment maintains the current SSA value for each variable. As the compiler traverses control flow, different branches produce different environment states.

### 3.2 Phi Node Insertion Strategy

At all control flow merge points (if/else merge, loop header, loop exit), apply the following rule uniformly:

For each variable `x`:
1. Collect all incoming environment values: `incomings = {outEnv[pred][x] | pred ∈ predecessors}`
2. If all incoming values are identical: `env_merge[x] = that_value`
3. If incoming values differ: Insert `x_phi = phi(type, incomings)` at block entry, then `env_merge[x] = x_phi`

**Key Principle:** This is the foundation of the custom SSA implementation. All other lowering logic follows standard patterns.

---

## 4. Control Flow Lowering Patterns

Three primary patterns cover all control flow constructs.

### 4.1 If-Else Pattern

**Precondition:** Current block `cur` with environment `env_cur`

**Steps:**

1. Evaluate condition: `v_cond = emitExpr(cond)`
2. Create blocks: `then_bb`, `else_bb`, `merge_bb`
3. Terminate current block: `cur.terminator = condbr(v_cond, then_bb, else_bb)`

**Then branch:**
- Enter `then_bb` with `env_then = env_cur`
- Emit then-branch statements → `out_env_then`
- If no early return: `br merge_bb`

**Else branch:**
- If no else clause: `out_env_else = env_cur`
- Otherwise, process else statements → `out_env_else`
- If no early return: `br merge_bb`

**Merge block:**
- `env_merge = phiMerge(out_env_then, out_env_else)`
- Continue emitting subsequent statements in `merge_bb`

### 4.2 While Loop Pattern (with Break/Continue)

**Blocks:** `header_bb` (condition evaluation), `body_bb`, `exit_bb`

**Steps:**

1. Branch to header: `cur.br(header_bb)`
2. **Reserve phi nodes at `header_bb` start (to be filled later)**
3. In `header_bb`:
   - Evaluate condition: `v_cond = emitExpr(cond)`
   - Branch: `condbr(v_cond, body_bb, exit_bb)`

4. In `body_bb`:
   - Push loop targets: `break_target = exit_bb`, `continue_target = header_bb`
   - Emit body statements → `out_env_body`
   - If no early return/break: `br header_bb`

5. **Backfill phi nodes in `header_bb`:**
   - For each variable `x` that may change in the loop:
     - Incoming from preheader (before loop): `env_cur[x]`
     - Incoming from body feedback: `out_env_body[x]`
     - Create: `x_phi = phi i32 [(preheader, env_cur[x]), (body_bb, out_env_body[x])]`
     - Update: `env_header[x] = x_phi`

6. In `exit_bb`:
   - Collect all predecessors: header false-branch + all break branches
   - Apply `phiMerge` to produce `env_exit`

**Implementation Tip:** Maintain a list of (predecessor block, outgoing environment) pairs for the exit block. Upon reaching exit, perform a single phi merge operation.

### 4.3 Short-Circuit Logical Operators

**For `a && b`:**

1. Evaluate `a`: `v_a = emitExpr(a)`
2. Create blocks: `eval_b`, `false_bb`, `merge`
3. Branch: `condbr(v_a, eval_b, false_bb)`
4. In `eval_b`:
   - Evaluate `b`: `v_b = emitExpr(b)`
   - Branch: `br merge`
5. In `false_bb`:
   - Branch: `br merge`
6. In `merge`:
   - `v_result = phi i1 [(false_bb, const_i1(false)), (eval_b, v_b)]`

**For `a || b`:**

1. Branch: `condbr(v_a, true_bb, eval_b)`
2. In `merge`:
   - `v_result = phi i1 [(true_bb, const_i1(true)), (eval_b, v_b)]`

---

## 5. IR to LLVM Lowering

The custom IR is mechanically mapped to LLVM IR through direct correspondence:

| Custom IR | LLVM IR |
|---|---|
| `add`, `sub`, `mul` | LLVM `add`, `sub`, `mul i32` |
| `sdiv`, `srem` | LLVM `sdiv`, `srem i32` |
| `icmp(cond)` | LLVM `icmp cond i32` |
| `not` | LLVM bitwise NOT |
| `phi` | LLVM `phi` |
| `br`, `condbr`, `ret` | LLVM terminator instructions |
| `call` | LLVM `call` |

### Implementation Strategy

1. Use **LLVM C++ API** to construct `Module`, `Function`, and `BasicBlock` structures
2. Maintain a **value mapping**: `map<CustomIR_ValueID, llvm::Value*>`
3. **Two-pass generation:**
   - **Pass 1:** Create all functions and basic blocks
   - **Pass 2:** Fill in instructions (create phi nodes first, populate incoming values later)

---

## 6. End-to-End Compilation Pipeline

The final compilation pipeline produces an executable binary through the following steps:

1. **Compiler Output:** Generate object file (`.o`) using LLVM `TargetMachine::emit`
2. **Linking:** Link with runtime library: `clang out.o runtime.o -o a.out`

### Runtime Library

The runtime library provides minimal support:

```c
void print_i32(int32_t value);
void print_bool(uint8_t value);
```

---

## 7. Implementation Milestones

The following milestones establish a strict execution order to progressively validate the compiler's core functionality.

### Milestone 1: Basic Functions
- **Goal:** Functions and constant returns
- **Features:** Top-level function definitions, return statements with constants
- **Validation:** Compile and execute simple functions returning constants

### Milestone 2: Variables and Arithmetic
- **Goal:** Local variables and arithmetic expressions
- **Features:** `val`/`var` declarations, `+`, `-`, `*`, `/`, `%`, built-in `print_i32()`
- **Validation:** Demonstrate variable assignment and arithmetic operations

### Milestone 3: Conditional Branching
- **Goal:** If-else statements with phi node merging
- **Features:** If-else control flow, phi-based SSA merging
- **Validation:** Verify correct variable values after branches converge
- **Significance:** Establishes the minimal SSA-CFG closure

### Milestone 4: Loop Constructs
- **Goal:** While loops with break/continue support
- **Features:** While condition evaluation, body iteration, header/exit phi merging
- **Validation:** Test loop termination, break/continue behavior, variable updates across iterations

### Milestone 5: Short-Circuit Evaluation
- **Goal:** Logical operators with short-circuit semantics
- **Features:** `&&` and `||` with proper short-circuit behavior via selective block evaluation
- **Validation:** Verify that right operand is not evaluated when unnecessary
- **Significance:** Completes a functional CFG compiler with full semantic completeness

