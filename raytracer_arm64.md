# Parametric fixed-point raytracer to AArch64

`raytracer_arm64.cpp` expands the semantic-description compiler into a small
raytracer compiler instance.

The high-level description contains:

- image width and height;
- sphere center and radius;
- C/AArch64 entry symbol;
- syscall register, number, and instruction.

For the sample description, the compiler emits a 64×48 nested render loop and
a fixed-point orthographic sphere kernel. Each pixel calls:

```text
raytrace_pixel(x, y) -> intensity
pixel_sink(intensity, x, y)
```

The output hook is explicit so a C variant can choose PPM, framebuffer, or
`write(2)` serialization without changing the ray-intersection kernel.

Build and verify:

```sh
g++ -std=c++23 -Wall -Wextra -pedantic -O2 raytracer_arm64.cpp -o raytracer_arm64
./raytracer_arm64 > raytracer_arm64.s
```

The executable reports `raytracer staged equivalence: PASS`. The generated
assembly contains `raytrace`, `.Lrow`, `.Lcolumn`, `raytrace_pixel`,
`pixel_sink`, and `sys_write`.
