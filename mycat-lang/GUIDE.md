# mycat-lang Guide

This guide covers how to write and run programs in `mycat-lang`.

## 1. Run a script

From `mycat-lang/`:

```bash
make
./bin/mycat-lang path/to/script.mcl
```

## 2. Core syntax

`mycat-lang` is line-based and uses `end` to close blocks.

```txt
let count = 1

while count <= 3
print count
let count = count + 1
end
```

## 3. Statements

### Variable assignment

```txt
let x = 10
let y = x + 5
```

### Print

Print number/expression:

```txt
print x * 2
```

Print string:

```txt
print "hello 🐱"
```

### If / else

```txt
if x > 0
print "positive"
else
print "zero-or-negative"
end
```

### While

```txt
let n = 3
while n > 0
print n
let n = n - 1
end
```

## 4. Expressions

Supported operators:

- Arithmetic: `+ - * /`
- Comparison: `== != < <= > >=`
- Grouping: `( ... )`
- Unary minus: `-x`

All values are integers.

## 5. Comments

Use `#` for single-line comments:

```txt
# This is ignored
let x = 1
```

## 6. Full example

```txt
let x = 1
print "🐱 mycat-lang"

while x <= 5
print x
let x = x + 1
end

if x == 6
print "done"
else
print "unexpected"
end
```

## 7. Common errors

- `undefined variable 'name'`: used a variable before assigning it.
- `division by zero`: divisor evaluated to `0`.
- `expected 'end'`: opened `if` or `while` without closing it.
- `unexpected token in statement`: invalid line/keyword usage.

## 8. Current limits (v1)

- No functions
- No booleans type (conditions use integer truthiness: `0` is false, nonzero is true)
- No strings in arithmetic
- No REPL (file execution only)
