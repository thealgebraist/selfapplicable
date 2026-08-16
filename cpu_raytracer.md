# Native CPU raytracer backend

`cpu_ray_codegen.cpp` specializes the raytracer semantic description directly
into x86-64 AT&T assembly. The target is assembled with `as --64` and linked
with `ld`; no C compiler is used for the target.

Pipeline:

```text
RaySpec -> quote/unquote -> generated_raytracer.s
        -> generated_raytracer.o
        -> generated_raytracer
        -> render.ppm
```

Run:

```sh
g++ -std=c++23 -Wall -Wextra -pedantic -O2 cpu_ray_codegen.cpp -o cpu_ray_codegen
./cpu_ray_codegen > generated_raytracer.s
as --64 generated_raytracer.s -o generated_raytracer.o
ld generated_raytracer.o -o generated_raytracer
./generated_raytracer
```

The executable was run successfully and produced a valid `96 x 64` binary PGM
image at `render.pgm`. Direct and quoted-description generation are checked for
byte-identical assembly output.
