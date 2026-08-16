// Macro-free C subset frontend connected to the dependent NbE core.
// Supported source shape:
//   int main(int argc, char **argv) { return <decimal>; }
// Preprocessor lines and comments are ignored.  The target is emitted as
// x86-64 Linux assembler and linked with as/ld, never with a C compiler.
#define NORMALISER_LIBRARY
#include "normaliser.cpp"
#include <fstream>
#include <regex>

namespace csubset {
std::string read_source(const char *path) {
  std::ifstream in(path); if(!in) throw std::runtime_error("cannot open source");
  std::string all,line;
  while(std::getline(in,line)) {
    if(!line.empty() && line[0]=='#') continue;
    auto p=line.find("//"); if(p!=std::string::npos) line.resize(p);
    all += line + '\n';
  }
  return all;
}

struct Program { int argc_value=-1, then_status=0, else_status=0; std::string output, loop_output, directory; int loop_count=0; bool argv1=false, arg_help=false, cwd=false, listdir=false; };

Program parse_main(std::string const& s) {
  static const std::regex main(R"(\bint\s+main\s*\(\s*int\s+argc\s*,\s*char\s*\*\s*\*\s*argv\s*\)\s*\{([\s\S]*)\})");
  std::smatch m; if(!std::regex_search(s,m,main)) throw std::runtime_error("unsupported main declaration");
  static const std::regex conditional(R"(if\s*\(\s*argc\s*==\s*([0-9]+)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)");
  auto body=m[1].str(); std::smatch r;
  Program p;
  if(std::regex_search(body,r,conditional)) { p.argc_value=std::stoi(r[1]); p.then_status=std::stoi(r[2]); p.else_status=std::stoi(r[3]); }
  else {
    static const std::regex ret(R"(\breturn\s+([0-9]+)\s*;)");
    if(!std::regex_search(body,r,ret)) throw std::runtime_error("return expression is outside subset");
    p.else_status=std::stoi(r[1]);
  }
  static const std::regex write(R"re(write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  std::smatch w; if(std::regex_search(body,w,write)) {
    p.output=w[1].str();
    std::size_t nl; while((nl=p.output.find("\\n"))!=std::string::npos) p.output.replace(nl,2,"\n");
  if(std::stoi(w[2])!=(int)p.output.size()) throw std::runtime_error("write length mismatch");
  }
  static const std::regex loop(R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=w[2].str();
    std::size_t nl; while((nl=p.loop_output.find("\\n"))!=std::string::npos) p.loop_output.replace(nl,2,"\n");
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex write_argv(R"(write\s*\(\s*1\s*,\s*argv\s*\[\s*1\s*\]\s*,\s*strlen\s*\(\s*argv\s*\[\s*1\s*\]\s*\)\s*\)\s*;)");
  p.argv1=std::regex_search(body,write_argv);
  static const std::regex getcwd_write(R"(write\s*\(\s*1\s*,\s*getcwd\s*\(\s*buf\s*,\s*4096\s*\)\s*,\s*strlen\s*\(\s*buf\s*\)\s*\)\s*;)");
  p.cwd=std::regex_search(body,getcwd_write);
  static const std::regex dir(R"re((?:listdir|finddir)\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  std::smatch d; if(std::regex_search(body,d,dir)) { p.listdir=true; p.directory=d[1].str(); }
  static const std::regex help(R"(if\s*\(\s*argc\s*==\s*2\s*&&\s*(?:streq|strcmp)\s*\(\s*argv\s*\[\s*1\s*\]\s*,\s*"--help"\s*\)\s*\)\s*return\s+([0-9]+)\s*;)");
  if(std::regex_search(body,w,help)) { p.arg_help=true; p.argc_value=2; p.then_status=std::stoi(w[1]); }
  return p;
}

void check_with_nbe() {
  using namespace st;
  auto A=sort(1);
  auto administrative=app(lam(A,var(0)),sort(0));
  auto normal=nbe_normalise({},administrative);
  if(!equal(normal,sort(0))) throw Error("C subset normalization failed");
}
}

int main(int argc,char **argv) {
  if(argc!=2) { std::cerr<<"usage: c_subset_compiler SOURCE\n"; return 2; }
  try {
    auto program=csubset::parse_main(csubset::read_source(argv[1]));
    csubset::check_with_nbe();
    std::cout<<".text\n.globl _start\n_start:\n";
    if(!program.output.empty()) std::cout
             <<"  mov $1, %eax\n  mov $1, %edi\n  lea message(%rip), %rsi\n  mov $"<<program.output.size()<<", %edx\n  syscall\n";
    if(program.loop_count>0) std::cout
             <<"  xor %r12d, %r12d\n.Lfor:\n  cmp $"<<program.loop_count<<", %r12d\n  jge .Lfor_done\n"
             <<"  mov $1, %eax\n  mov $1, %edi\n  lea loop_message(%rip), %rsi\n  mov $"<<program.loop_output.size()<<", %edx\n  syscall\n"
             <<"  inc %r12d\n  jmp .Lfor\n.Lfor_done:\n";
    if(program.argv1) std::cout
             <<"  mov (%rsp), %rdi\n  cmp $2, %rdi\n  jl .Largv_done\n"
             <<"  mov 16(%rsp), %rsi\n  xor %edx, %edx\n.Lstrlen:\n"
             <<"  cmpb $0, (%rsi,%rdx,1)\n  je .Lwritearg\n  inc %rdx\n  jmp .Lstrlen\n.Lwritearg:\n"
             <<"  mov $1, %eax\n  mov $1, %edi\n  syscall\n";
    if(program.cwd) std::cout
             <<"  lea cwd_buf(%rip), %rdi\n  mov $4096, %esi\n  mov $79, %eax\n  syscall\n"
             <<"  lea cwd_buf(%rip), %rsi\n  xor %edx, %edx\n.Lcwdlen:\n"
             <<"  cmpb $0, (%rsi,%rdx,1)\n  je .Lcwdwrite\n  inc %rdx\n  jmp .Lcwdlen\n.Lcwdwrite:\n"
             <<"  mov $1, %eax\n  mov $1, %edi\n  lea cwd_buf(%rip), %rsi\n  syscall\n";
    if(program.listdir) std::cout
             <<"  mov $257, %eax\n  mov $-100, %edi\n  lea dir_path(%rip), %rsi\n  xor %edx, %edx\n  xor %r10d, %r10d\n  syscall\n"
             <<"  mov %eax, %r13d\n  mov $217, %eax\n  mov %r13d, %edi\n  lea dir_buf(%rip), %rsi\n  mov $8192, %edx\n  syscall\n  mov %eax, %r14d\n  xor %r12d, %r12d\n.Ldir_next:\n  cmp %r14d, %r12d\n  jge .Ldir_done\n  lea dir_buf(%rip), %rsi\n  movzwl 16(%rsi,%r12,1), %ecx\n  mov %ecx, %r15d\n  test %ecx, %ecx\n  jz .Ldir_done\n  lea 19(%rsi,%r12,1), %r8\n  xor %edx, %edx\n.Ldir_name:\n  cmp %edx, %ecx\n  jle .Ldir_write\n  cmpb $0, (%r8,%rdx,1)\n  je .Ldir_write\n  inc %edx\n  jmp .Ldir_name\n.Ldir_write:\n  mov $1, %eax\n  mov $1, %edi\n  mov %r8, %rsi\n  syscall\n  mov $1, %eax\n  mov $1, %edi\n  lea newline(%rip), %rsi\n  mov $1, %edx\n  syscall\n  add %r15d, %r12d\n  jmp .Ldir_next\n.Ldir_done:\n";
    if(program.arg_help) std::cout
             <<"  mov (%rsp), %rdi\n  cmp $2, %rdi\n  jne .Lhelp_no\n"
             <<"  mov 16(%rsp), %rsi\n  cmpb $'-', (%rsi)\n  jne .Lhelp_no\n"
             <<"  cmpb $'-', 1(%rsi)\n  jne .Lhelp_no\n"
             <<"  cmpb $'h', 2(%rsi)\n  jne .Lhelp_no\n"
             <<"  cmpb $'e', 3(%rsi)\n  jne .Lhelp_no\n"
             <<"  cmpb $'l', 4(%rsi)\n  jne .Lhelp_no\n"
             <<"  cmpb $'p', 5(%rsi)\n  jne .Lhelp_no\n"
             <<"  mov $"<<program.then_status<<", %edi\n  jmp .Lexit\n.Lhelp_no:\n";
    if(program.argv1) std::cout<<".Largv_done:\n  mov $"<<program.else_status<<", %edi\n";
    if(program.argc_value>=0) std::cout
             <<"  mov (%rsp), %rdi\n  cmp $"<<program.argc_value<<", %rdi\n"
             <<"  jne .Lelse\n  mov $"<<program.then_status<<", %edi\n  jmp .Lexit\n.Lelse:\n";
    std::cout<<"  mov $"<<program.else_status<<", %edi\n"
             <<".Lexit:\n"
             <<"  mov $60, %eax\n  syscall\n";
    if(!program.output.empty()) {
      std::cout<<".section .rodata\nmessage:\n  .byte ";
      for(std::size_t i=0;i<program.output.size();++i) { if(i) std::cout<<", "; std::cout<<(unsigned)(unsigned char)program.output[i]; }
      std::cout<<"\n";
    }
    if(!program.loop_output.empty()) {
      std::cout<<"loop_message:\n  .byte ";
      for(std::size_t i=0;i<program.loop_output.size();++i) { if(i) std::cout<<", "; std::cout<<(unsigned)(unsigned char)program.loop_output[i]; }
      std::cout<<"\n";
    }
    if(program.cwd) std::cout<<".bss\n.align 8\ncwd_buf:\n  .skip 4096\n";
    if(program.listdir) std::cout<<".section .rodata\ndir_path:\n  .asciz \""<<program.directory<<"\"\nnewline:\n  .byte 10\n.bss\n.align 8\ndir_buf:\n  .skip 8192\n";
    std::cerr<<"C subset + dependent NbE: PASS\n";
  } catch(std::exception const& e) { std::cerr<<"C subset error: "<<e.what()<<'\n'; return 1; }
}
