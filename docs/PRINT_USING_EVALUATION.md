# PRINT USING Evaluation (Full Mask Semantics)

## Scope

This document evaluates what it would take to add **GW-BASIC-compatible `PRINT USING`** behavior to WBASIC, including numeric and string mask semantics, parser/runtime integration, and regression coverage.

## Current WBASIC Behavior (Baseline)

- `PRINT` currently supports:
  - plain string/numeric print-list items,
  - `,` (zone spacing), `;` (concatenation),
  - `TAB(n)` and `SPC(n)`.
- Numeric output currently uses generic formatting (`%.0f` for integral values, `%.12g` otherwise).
- There is no `USING` branch in `exec_print`.

Implication: `PRINT USING "..."; ...` is not implemented and will not provide GW-BASIC mask formatting.

---

## Target Semantics for “Full Mask Semantics”

The following are the key GW-BASIC-compatible behaviors to implement.

### 1) Statement Forms

- `PRINT USING format$; exprlist`
- `PRINT USING format$, exprlist`
- `LPRINT USING ...` (future extension if/when `LPRINT` exists)
- Optional future parity: `PRINT #n, USING ...` (if matching target dialect behavior)

### 2) Format String Scanning Model

A robust model is:

1. Parse and evaluate `format$` once.
2. Compile it into a sequence of tokens:
   - literal text,
   - **numeric fields**,
   - **string fields**.
3. Consume expressions left-to-right, applying each expression to the next compatible field.
4. Emit literals exactly as specified by format.
5. When expressions remain but fields are exhausted, **restart scanning from format start** (classic BASIC behavior).

### 3) Numeric Field Semantics

Numeric masks should support the canonical symbols:

- `#` digit placeholder
- `.` decimal point placement
- `,` thousands grouping insertion points
- `+` sign control (leading/trailing variants)
- `-` trailing minus style when applicable
- `$` currency marker
- `*` fill behavior (`**` leading fill variant)
- `^^^^` scientific notation mode

Behavioral requirements:

- Round to fractional width specified by mask.
- Right-justify into field width unless mask semantics imply otherwise.
- Overflow should emit `%` (or dialect-correct overflow marker) for the full numeric field width.
- Correct interaction among signs, currency, fill, and grouping.

### 4) String Field Semantics

String masks should support:

- `!` one-character field (first character only)
- `\   \` fixed-width literal-delimited string field
- `&` variable-width string output

Behavioral requirements:

- Truncation/padding rules per field type.
- Type mismatch should raise GW-BASIC-like runtime error where expected.

### 5) Mixed Literal + Field Format

The format string can include static text around fields, for example:

- `"Total: $$###.##"`
- `"Name: \      \  Score: ###"`

Literal characters are emitted as-is between formatted substitutions.

### 6) Print-List Interaction

After introducing `USING` mode, separators still matter:

- `;` suppresses newline at statement end.
- `,` performs zone-based movement outside `USING` expansion boundaries.
- Empty arguments and trailing separators should follow existing `PRINT` policy unless dialect parity requires override.

---

## Parser/Runtime Design for WBASIC

## A) Parse-Level Integration

In `exec_print`:

1. Detect `USING` at print-list start (and optional dialect-accepted positions).
2. Parse `format$` expression as string.
3. Require separator (`;` or `,`) before value list.
4. Route remaining list to `exec_print_using(...)`.

## B) Internal Data Structures

Suggested internal model:

```c
typedef enum {
    FMT_LIT,
    FMT_NUM,
    FMT_STR
} PrintUsingTokenKind;

typedef struct {
    PrintUsingTokenKind kind;
    char *lit;
    // numeric descriptor
    int width;
    int frac_digits;
    bool use_grouping;
    bool leading_plus;
    bool trailing_sign;
    bool dollar;
    bool star_fill;
    bool scientific;
    // string descriptor
    int str_width;      // for \    \
    char str_mode;      // '!', '\\', '&'
    // source span for diagnostics
    int src_begin;
    int src_end;
} PrintUsingToken;
```

Compile once per statement execution; reuse when cycling format for extra arguments.

## C) Formatter Pipeline

1. **Compile** mask string to tokens.
2. **Format value** by token type:
   - numeric formatter,
   - string formatter.
3. **Emit** literal/token output through existing `out_append` path.
4. Preserve existing newline/separator handling contract.

## D) Error Mapping

Maintain GW-BASIC-style runtime messages where possible:

- bad/malformed mask => syntax/runtime parity message,
- type mismatch between mask-field and value,
- illegal/unsupported mask combinations.

---

## Compatibility Risks & Edge Cases

1. **Cycling behavior** when args exceed fields is easy to get subtly wrong.
2. **Overflow marker** details differ across BASIC variants.
3. **Scientific (`^^^^`) semantics** can conflict with sign/currency/fill interactions.
4. **Mask literals containing escaped backslashes** need careful scanner handling.
5. **Rounding ties** must be consistent with existing numeric policy.
6. `TAB/SPC` adjacency and implicit-item behavior currently in `PRINT` must remain intact.

---

## Test Plan (Required for Full-Semantics Claim)

Add dedicated tests under `tests/test cases/` for:

1. **Basic numeric masks**
   - `##.##`, `###`, `#.####`
2. **Grouping/sign/currency/fill**
   - `#,###.##`, `+###.##`, `###.##-`, `$$###.##`, `**###.##`
3. **Scientific notation**
   - `#.##^^^^`
4. **String masks**
   - `!`, `\    \`, `&`
5. **Mixed literal text**
   - e.g., `"X="###" Y="###` patterns
6. **Cycling format**
   - fewer fields than arguments
7. **Errors**
   - malformed masks,
   - type mismatches,
   - overflow rendering
8. **Separator interaction**
   - trailing `;`, `,`, empty items, TAB/SPC adjacency around USING paths.

Recommended strategy:

- golden-output tests in existing torture harness style,
- small focused `.bas` programs for each mask family,
- one aggregate regression file covering mixed scenarios.

---


## Implementation Readiness (Prep for the Actual Update)

To prepare for implementation, complete this short pre-flight checklist before coding:

1. **Confirm current parser entry points**
   - Locate `exec_print` and the token parser path that currently handles `PRINT` list items.
   - Verify where `USING` can be recognized without regressing normal `PRINT` behavior.

2. **Define the first shippable milestone (MVP)**
   - MVP should include: `PRINT USING format$; exprlist` with numeric `#`/`.` masks, rounding, width, and overflow marker behavior.
   - Explicitly defer sign/currency/fill/scientific/string-mask semantics to follow-up milestones.

3. **Add formatter scaffolding before feature depth**
   - Introduce a `compile_print_using_format(...)` path that returns tokenized literals + fields.
   - Add an execution loop that supports format cycling when arguments exceed fields.

4. **Lock down compatibility decisions up front**
   - Decide and document exact overflow marker policy (`%` width fill vs dialect variant).
   - Decide error text mapping strategy (exact GW-BASIC text vs WBASIC-consistent equivalents).

5. **Create test harness slots first**
   - Add placeholder test files for numeric core, separator interaction, and cycling behavior.
   - Start with golden-output baselines so behavior changes are visible per phase.

### Suggested Work Breakdown (Implementation-Ready)

- **PR 1:** parser detection + format tokenizer skeleton + no-op/literal-safe execution path.
- **PR 2:** numeric core (`#`, `.`, width, rounding, overflow) + focused tests.
- **PR 3:** extended numeric symbols (`,`, `+`, `-`, `$`, `*`, `^^^^`) + interaction tests.
- **PR 4:** string masks (`!`, `\...\`, `&`) + mixed literal formatting + parity cleanup.

This preparation reduces implementation churn, keeps regressions isolated, and enables incremental parity tracking against GW-BASIC output expectations.

## Estimated Implementation Effort

- **Phase 1 (MVP numeric only)**: 1–2 days
  - `#`, `.`, rounding, width, overflow, basic integration
- **Phase 2 (full numeric semantics)**: +2–4 days
  - sign/currency/fill/group/scientific details and edge behavior
- **Phase 3 (string fields + parity polish)**: +1–2 days
- **Phase 4 (tests + parity fixes)**: +1–3 days

Total for credible “full mask semantics” parity: approximately **1–2 weeks** depending on strictness and how close to exact GW-BASIC behavior is required.

---

## Recommendation

Implement in phases with explicit milestone labels:

1. `PRINT USING` parser + token compiler scaffold.
2. Numeric core (`#`, `.`, width, rounding, overflow).
3. Extended numeric symbols and interactions.
4. String masks and mixed-format stress tests.
5. Parity hardening against known GW-BASIC outputs.

This reduces risk and allows incremental shipping while preserving existing `PRINT` behavior.
