# Four typed CLI tools from a minimal DSL

`cli_dsl.cpp` implements four classic command-line tools over a small typed DSL:

- `grep` with a statically captured literal pattern;
- `head -n` with a statically captured line count;
- `cut -d c -f n` with statically captured delimiter and field;
- `wc -l` with a typed `Text -> Count` result.

The `Program<In, Out>` index is the DSL's object-level type. Its node variant is
selected by the input/output pair, so impossible command/result combinations do
not compile. `interpret` executes the generic DSL. `normalize` canonicalizes the
program representation, and `specialize` removes the command dispatch by closing
over the static command parameters.

Build and run:

```sh
g++ -std=c++23 -Wall -Wextra -pedantic -O2 cli_dsl.cpp -o cli_dsl
./cli_dsl
```

Expected result:

```text
grep: PASS
head: PASS
cut: PASS
wc -l: PASS
all specialized CLI tools: PASS
```
