raytracer staged equivalence: PASS
.text
.global raytrace
raytrace:
  stp x29, x30, [sp, #-16]!
  mov x29, sp
  mov x19, #0
.Lrow:
  cmp x19, #48
  b.ge .Lfinish
  mov x20, #0
.Lcolumn:
  cmp x20, #64
  b.ge .Lnextrow
  mov x0, x20
  mov x1, x19
  bl raytrace_pixel
  mov x1, x20
  mov x2, x19
  bl pixel_sink
  add x20, x20, #1
  b .Lcolumn
.Lnextrow:
  add x19, x19, #1
  b .Lrow
.Lfinish:
  ldp x29, x30, [sp], #16
  ret

// x0=x, x1=y; x0=1 on a sphere hit, else 0
raytrace_pixel:
  sub x2, x0, #32
  mul x2, x2, x2
  sub x3, x1, #24
  mul x3, x3, x3
  add x2, x2, x3
  mov x3, #400
  cmp x2, x3
  cset x0, lt
  ret

// Output policy: pixel_sink(intensity, x, y) is the C ABI hook.
pixel_sink:
  ret

sys_write:
  mov x8, #64
  svc #0
  ret
