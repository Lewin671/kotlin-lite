# Intermediate Representation (IR)

This document expands on the **custom SSA-CFG IR** described in `docs/architecture.md`. It clarifies the IR hierarchy, core instructions, SSA environment tracking, control-flow idioms, and the lowering rhythm that connects this IR to LLVM.

## IR Overview

- **Position in the pipeline:** `Source → AST → Semantic Analysis → Custom IR → LLVM IR → Object → Executable`.
- **Key goals:** keep the IR minimal, SSA-based, and close enough to LLVM that the final lowering is mechanical.
- **Structure:** `Module → Function[] → BasicBlock[] → Instruction[]` with a mandatory terminator per block.

## Type System

| Custom Name | Meaning | LLVM Equivalent |
|-------------|---------|-----------------|
| `i32` | 32-bit signed integer | `i32` |
| `i1` | Boolean value | `i1` |
| `void` | Unit / no-return functions | `void` |

All SSA values carry exactly one of the above types, which keeps the IR statically typed and easy to validate before LLVM conversion.

## Instruction Set

### Value-Producing Instructions

| Instruction | Semantics | Signature |
|-------------|-----------|-----------|
| `const_i32(value)` | Immediate 32-bit constant | `→ i32` |
| `const_i1(value)` | Immediate boolean constant | `→ i1` |
| `add`, `sub`, `mul` | Integer arithmetic | `i32, i32 → i32` |
| `sdiv`, `srem` | Signed division and remainder | `i32, i32 → i32` |
| `icmp(cond)` | Comparison (eq/ne/slt/sle/sgt/sge) | `i32, i32 → i1` |
| `not` | Logical negation | `i1 → i1` |
| `phi(type, incomings)` | Merge differing SSA values at joins | `→ type` |
| `call(fn, args...)` | Function invocation | `→ (i32 | i1 | void)` |

### Terminators

| Instruction | Purpose |
|-------------|---------|
| `br(target)` | Unconditional jump to another block |
| `condbr(cond, then, else)` | Branch based on a boolean value |
| `ret([val])` | Return control (and optional value) from a function |

Each basic block ends with exactly one terminator before new blocks are emitted.

## SSA Environment Model

The IR emitter maintains an **environment**: `env : VariableName → SSAValue`. Every time the emitter visits a statement or expression, it updates `env` with the SSA value that represents the value of that variable in the current control flow path.

### Phi Node Strategy

At merge points (e.g., `if/else`, loop exits, short-circuit joins), a new environment `env_merge` is built from incoming environments.

1. Gather incoming values for each variable: `incomings = {env_pred[x] | pred ∈ predecessors}`.
2. If all values are identical, `env_merge[x]` reuses the shared SSA value.
3. If values differ, emit `phi(type, [(pred, value), ...])` at the merge block entry and set `env_merge[x]` to that phi value.

This process ensures each variable name maps to at most one SSA value per block and mirrors LLVM’s SSA semantics.

## Control Flow Patterns

### If / Else

1. Evaluate the condition → `v_cond`.
2. Create `then`, `else`, and `merge` blocks.
3. Terminate the current block with `condbr(v_cond, then, else)`.
4. Emit each branch using a copy of the incoming environment, then add an unconditional branch to `merge`.
5. At `merge`, apply the phi strategy to reconcile `env_then` and `env_else`.

### While Loops (break / continue support)

Blocks: `preheader → header → body → exit`.

1. Jump from the current block to `header`.
2. Reserve phi nodes at the start of `header` for loop-carried variables.
3. Evaluate the condition in `header`, branch to `body` or `exit`.
4. Emit `body` with `break`/`continue` targets recorded. Loop back to `header` if no early exits.
5. After the body, update the phi nodes with the `body`’s environment entries (preheader vs. body feedback).
6. `exit` merges all incoming environments (including breaks) with the phi strategy.

### Short-Circuit Logic

For `a && b` and `a || b`, the IR uses explicit blocks to model evaluation order:

- `a && b`: Branch to `eval_b` only when `a` is true; `false` goes directly to a merge block with `const_i1(false)`. Merge uses a phi between the dangling `false` path and `b`.
- `a || b`: Branch to `true` block when `a` is true; otherwise evaluate `b`. Merge phi merges `true` constant and `b`.

Phi nodes keep the final boolean result SSA-safe and avoid evaluating `b` when not necessary.

## Lowering to LLVM

Because the custom IR closely mirrors LLVM, the lowering process is mostly a **mechanical translation**:

1. **Two-Pass Generation**
   - **Pass 1:** Create LLVM `Module` and `Function` objects along with all basic blocks so branching targets exist.
   - **Pass 2:** Emit instructions, instantiating phi nodes first and populating incoming edges afterward.
2. Maintain a `map<CustomIR_ValueID, llvm::Value*>` so each SSA value resolves to a concrete LLVM value.
3. Instruction mappings are direct (`add` → `llvm::add`, `icmp` → `llvm::icmp`, etc.), and terminators map to LLVM terminators.

The emitter never uses `alloca`/`load`/`store` for local variables; all data flows through SSA values until the final code is emitted.

## Validation Notes

- Built-in functions `print_i32` and `print_bool` remain externally linked through the runtime.
- Test coverage should exercise phi merges after conditional and loop constructs to ensure SSA correctness.
- When emitting loops or nested branches, use assertions to verify that every block has a terminator and that phi incomings list every predecessor.

Keeping the IR minimal also makes it easier to spot regressions before reaching LLVM.
