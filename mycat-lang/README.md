# mycat-lang

`mycat-lang` is a tiny interpreted language with custom `end`-block syntax.

Full language walkthrough: **[GUIDE.md](GUIDE.md)**.

## Features (v1)

- Variables (`let`)
- Integer arithmetic (`+ - * /`)
- Comparisons (`== != < <= > >=`)
- `if / else / end`
- `while / end`
- `print` for expressions and strings

## Build

```bash
make
```

Binary:

```bash
./bin/mycat-lang
```

## Syntax

```txt
let x = 1
print x + 2

while x < 5
print x
let x = x + 1
end

if x == 5
print "🐱 done"
else
print "not done"
end
```

Notes:

- Blocks are closed with `end`.
- `print` accepts either an expression or a string literal.
- Lines can contain comments with `#`.

## Run

```bash
./bin/mycat-lang examples/demo.mcl
```
