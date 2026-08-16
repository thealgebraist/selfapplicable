#include <iostream>
#include <string>
#include <utility>

// Native-host backend: specialize the ray description directly into x86-64
// AT&T assembly. No C compiler is involved in the target build.
struct RaySpec { int width,height,cx,cy,radius; std::string output; };
template<class T> struct Code { T value; };
Code<RaySpec> quote(RaySpec x){return {std::move(x)};}
const RaySpec& unquote(const Code<RaySpec>& x){return x.value;}

std::string generate(const RaySpec& s){
  const auto pixels=s.width*s.height;
  const auto header="P5\n"+std::to_string(s.width)+" "+std::to_string(s.height)+"\n255\n";
  return ".section .text\n.global _start\n_start:\n"
    "  lea buffer(%rip), %r8\n  xor %r12d, %r12d\n.Lrow:\n"
    "  cmp $"+std::to_string(s.height)+", %r12d\n  jge .Lopen\n  xor %r13d, %r13d\n.Lcol:\n"
    "  cmp $"+std::to_string(s.width)+", %r13d\n  jge .Lnextrow\n"
    "  mov %r13d, %eax\n  sub $"+std::to_string(s.cx)+", %eax\n  imul %eax, %eax\n  mov %r12d, %ecx\n  sub $"+std::to_string(s.cy)+", %ecx\n  imul %ecx, %ecx\n  add %ecx, %eax\n  cmp $"+std::to_string(s.radius*s.radius)+", %eax\n  setl %al\n  neg %al\n  and $255, %al\n  mov %al, (%r8)\n  inc %r8\n  inc %r13d\n  jmp .Lcol\n.Lnextrow:\n  inc %r12d\n  jmp .Lrow\n.Lopen:\n"
    "  mov $257, %eax\n  mov $-100, %edi\n  lea path(%rip), %rsi\n  mov $577, %edx\n  mov $0644, %r10d\n  syscall\n  mov %eax, %r12d\n"
    "  mov $1, %eax\n  mov %r12d, %edi\n  lea header(%rip), %rsi\n  mov $"+std::to_string(header.size())+", %edx\n  syscall\n"
    "  mov $1, %eax\n  mov %r12d, %edi\n  lea buffer(%rip), %rsi\n  mov $"+std::to_string(pixels)+", %edx\n  syscall\n"
    "  mov $3, %eax\n  mov %r12d, %edi\n  syscall\n  mov $60, %eax\n  xor %edi, %edi\n  syscall\n"
    ".section .rodata\nheader: .ascii \"P5\\n"+std::to_string(s.width)+" "+std::to_string(s.height)+"\\n255\\n\"\n"
    "path: .asciz \""+s.output+"\"\n.section .bss\n.lcomm buffer, "+std::to_string(pixels)+"\n";
}
std::string staged(const Code<RaySpec>& c){return generate(unquote(c));}

int main(){
  RaySpec spec{96,64,48,32,24,"render.pgm"};
  auto q=quote(spec); auto direct=generate(spec), via_stage=staged(q);
  if(direct!=via_stage)return 1;
  std::cout<<via_stage;
}
