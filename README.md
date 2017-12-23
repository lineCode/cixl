<a href="https://liberapay.com/basic-gongfu/donate"><img alt="Donate using Liberapay" src="https://liberapay.com/assets/widgets/donate.svg"></a>

## cixl
#### a C-powered, minimalistic extension language

This project aims to produce a minimalistic extension language, or DSL substrate; in C. In a way, it's Lua taken one step further down the path of simplicity. The implementation is a hybrid interpreter/vm design, designed to be as fast as possible without compromising on transparency and flexibility.

### REPL
A primitive REPL is included, the executable weighs in at 200k without dependencies.

```
cixl v0.1

Press Return twice to eval input.

> 1 2 3
..
[1 2 3]

> quit
```

### Stack
The parameter stack is exposed to user code, just like in Forth.

```
> 4 5 dup
..
[1 2 3 4 5 5]

> zap
..
[1 2 3 4 5]

> cls
..
[]
```

### Expressions
But unlike Forth, functions scan forward until enough arguments are on the stack to allow reordering parameters and operations in user code to fit the problem being solved.

```
> 1 + 2
..
[3]

> 1 2 +
..
[3 3]

> + 1 2
..
[6 1 2]

> + +
..
[9]
```

### Variables
Named variables may be bound once per scope using the ```let:```-macro.

```
> let: foo 42;
..
[]

> $foo
..
[42]
```

### Lambdas
Braces quote contained code, which is then pushed on the stack.

```
> {1 2 3}
..
[Lambda(0x52d97d0:1)]

> call
..
[1 2 3]
```

### Scopes
Enclosing code in parens results in code being evaluated in a separate scope, the last value on the stack is automatically pushed on the parent stack on scope exit. The parent environment in reachable from within the scope, but variables set inside the scope are not visible from the outside.

```
> (1 2 3)
..
[3]

> let: foo 1; (let: foo 2; $foo) $foo
[2 1]
```

### Functions
The ```func:```-macro may be used to define named functions. An integer may be specified instead of argument type, which is then substituted for the actual type on evaluation.

```
> func: foo() 42; foo
..
[42]

> func: bar(x Int) $x + 35; cls bar 7
..
[42]

> func: baz(x y Int z 0) $x + $y + $z; cls baz 1 3 5
..
[9]

```

### License
GPLv3

Give me a yell if something is unclear, wrong or missing. And please do consider helping out with a [donation](https://liberapay.com/basic-gongfu/donate) if you find this useful, every contribution counts.