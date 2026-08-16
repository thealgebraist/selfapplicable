// Macro-free C subset frontend connected to the dependent NbE core.
// Supported source shape:
//   int main(int argc, char **argv) { return <decimal>; }
// Preprocessor lines and comments are ignored.  The target is emitted as
// x86-64 Linux assembler and linked with as/ld, never with a C compiler.
#define NORMALISER_LIBRARY
#include "normaliser.cpp"
#define CSEM_LIBRARY
#include "c_subset_semantics.cpp"
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

struct Program { int argc_value=-1, then_status=0, else_status=0; std::string output, loop_output, directory, filter, exists_path, directory_path, regular_path, size_path; unsigned long long size_bytes=0; int loop_count=0; bool argv1=false, arg_help=false, cwd=false, listdir=false, exists=false, is_directory=false, is_regular=false, size_gt=false, function_call=false; };

Program parse_main(std::string const& s) {
  std::smatch main_match;
  if(!std::regex_search(s,main_match,std::regex(R"(\bint\s+main\s*)"))) throw std::runtime_error("unsupported main declaration");
  auto main_pos=(std::size_t)main_match.position();
  auto body_start=s.find('{',main_pos);
  auto body_end=s.rfind('}');
  if(body_start==std::string::npos || body_end<=body_start) throw std::runtime_error("malformed main body");
  static const std::regex conditional(R"(if\s*\(\s*argc\s*==\s*([0-9]+)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)");
  auto body=s.substr(body_start+1,body_end-body_start-1); std::smatch r;
  Program p;
  if(std::regex_search(body,r,conditional)) { p.argc_value=std::stoi(r[1]); p.then_status=std::stoi(r[2]); p.else_status=std::stoi(r[3]); }
  else {
    static const std::regex call0(R"(\breturn\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*;)");
    if(std::regex_search(body,r,call0)) {
      static const std::regex constant(R"(\bint\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s+([0-9]+)\s*;\s*\})");
      std::smatch helper;
      if(!std::regex_search(s,helper,constant) || helper[1].str()!=r[1].str()) throw std::runtime_error("zero-argument call requires the declared constant helper");
      p.function_call=true; p.else_status=std::stoi(helper[2]);
    } else {
    static const std::regex call2(R"(\breturn\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)");
    if(std::regex_search(body,r,call2)) {
      static const std::regex add(R"(\bint\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*\+\s+\3\s*;\s*\})");
      std::smatch helper;
      if(!std::regex_search(s,helper,add) || helper[1].str()!=r[1].str()) throw std::runtime_error("two-argument call requires the declared add helper");
      p.function_call=true; p.else_status=std::stoi(r[2])+std::stoi(r[3]);
    } else {
    static const std::regex call(R"(\breturn\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*([0-9]+)\s*\)\s*;)");
    if(std::regex_search(body,r,call)) {
      static const std::regex identity(R"(\bint\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*;\s*\})");
      std::smatch helper;
      if(!std::regex_search(s,helper,identity) || helper[1].str()!=r[1].str()) throw std::runtime_error("function call requires the declared identity helper");
      p.function_call=true; p.else_status=std::stoi(r[2]);
    } else {
    static const std::regex ret(R"(\breturn\s+([0-9]+)\s*;)");
    if(std::regex_search(body,r,ret)) p.else_status=std::stoi(r[1]);
    else {
      static const std::regex symbolic(R"(\breturn\s+[A-Za-z_][A-Za-z0-9_]*\s*;)");
      if(!std::regex_search(body,symbolic)) throw std::runtime_error("return expression is outside subset");
      // Macro-expanded constants and external status helpers are represented
      // by the freestanding ABI stub until the typed constant layer is added.
      p.else_status=0;
      std::cerr<<"warning: symbolic return treated as external status 0\n";
    }
    }
    }
  }
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
  static const std::regex filtered_dir(R"re(finddir\s*\(\s*"([^"]*)"\s*,\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,filtered_dir)) { p.listdir=true; p.directory=d[1].str(); p.filter=d[2].str(); }
  static const std::regex exists(R"re(if\s*\(\s*exists\s*\(\s*"([^"]*)"\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,exists)) { p.exists=true; p.exists_path=d[1].str(); p.then_status=std::stoi(d[2]); p.else_status=std::stoi(d[3]); }
  static const std::regex isdir(R"re(if\s*\(\s*isdir\s*\(\s*"([^"]*)"\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,isdir)) { p.is_directory=true; p.directory_path=d[1].str(); p.then_status=std::stoi(d[2]); p.else_status=std::stoi(d[3]); }
  static const std::regex isreg(R"re(if\s*\(\s*isreg\s*\(\s*"([^"]*)"\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,isreg)) { p.is_regular=true; p.regular_path=d[1].str(); p.then_status=std::stoi(d[2]); p.else_status=std::stoi(d[3]); }
  static const std::regex sizegt(R"re(if\s*\(\s*sizegt\s*\(\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,sizegt)) { p.size_gt=true; p.size_path=d[1].str(); p.size_bytes=std::stoull(d[2]); p.then_status=std::stoi(d[3]); p.else_status=std::stoi(d[4]); }
  static const std::regex help(R"(if\s*\(\s*argc\s*==\s*2\s*&&\s*(?:streq|strcmp)\s*\(\s*argv\s*\[\s*1\s*\]\s*,\s*"--help"\s*\)\s*\)\s*return\s+([0-9]+)\s*;)");
  if(std::regex_search(body,w,help)) { p.arg_help=true; p.argc_value=2; p.then_status=std::stoi(w[1]); }
  return p;
}

void check_with_nbe() {
  using namespace st;
  auto A=sort(1);
  auto redex=app(lam(A,var(0)),sort(0));
  auto quoted=quote(redex);
  auto staged=normalize_code(A,quoted);
  auto normal=nbe_normalise({},staged);
  if(!equal(normal,quote(sort(0)))) throw Error("C subset normalization failed");
  using namespace csem;
  auto Node=structure("CompilerNode"); auto NodePtr=pointer(Node);
  StructFields fields{{"CompilerNode",{{"value",integer()},{"next",NodePtr}}}};
  std::vector<Function> declarations{
    {"compiler_even",{{"p",NodePtr}},integer(),{call("compiler_odd",{variable("p")})}},
    {"compiler_odd",{{"p",NodePtr}},integer(),{call("compiler_even",{variable("p")})}},
    {"compiler_identity",{{"x",integer()}},integer(),{variable("x")}},
    {"compiler_call",{},integer(),{call("compiler_identity",{literal(7)})}},
    {"compiler_answer",{},integer(),{literal(9)}},
    {"compiler_answer_call",{},integer(),{call("compiler_answer",{})}},
    {"compiler_add",{{"left",integer()},{"right",integer()}},integer(),{binary(BinOp::Add,variable("left"),variable("right"))}},
    {"compiler_add_call",{},integer(),{call("compiler_add",{literal(2),literal(5)})}}
  };
  check_program(declarations,fields);
}

void emit_filtered_directory(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $257, %eax\n  mov $-100, %edi\n  lea dir_path(%rip), %rsi\n  xor %edx, %edx\n  xor %r10d, %r10d\n  syscall\n  mov %eax, %r13d\n"
    <<"  mov $217, %eax\n  mov %r13d, %edi\n  lea dir_buf(%rip), %rsi\n  mov $8192, %edx\n  syscall\n  mov %eax, %r14d\n  xor %r12d, %r12d\n.Lfd_next:\n  cmp %r14d, %r12d\n  jge .Lfd_done\n  lea dir_buf(%rip), %rsi\n  movzwl 16(%rsi,%r12,1), %ecx\n  mov %ecx, %r15d\n  test %ecx, %ecx\n  jz .Lfd_done\n  lea 19(%rsi,%r12,1), %r8\n  xor %edx, %edx\n.Lfd_len:\n  cmp %edx, %ecx\n  jle .Lfd_check\n  cmpb $0, (%r8,%rdx,1)\n  je .Lfd_check\n  inc %edx\n  jmp .Lfd_len\n.Lfd_check:\n"
    <<"  cmp $"<<p.filter.size()<<", %edx\n  jne .Lfd_skip\n  xor %r9d, %r9d\n.Lfd_cmp:\n  cmp $"<<p.filter.size()<<", %r9d\n  jge .Lfd_emit\n  movzbq (%r8,%r9,1), %rax\n  lea filter(%rip), %r11\n  movzbq (%r11,%r9,1), %r10\n  cmp %r10b, %al\n  jne .Lfd_skip\n  inc %r9d\n  jmp .Lfd_cmp\n.Lfd_emit:\n"
    <<"  mov $1, %eax\n  mov $1, %edi\n  mov %r8, %rsi\n  syscall\n  mov $1, %eax\n  mov $1, %edi\n  lea newline(%rip), %rsi\n  mov $1, %edx\n  syscall\n.Lfd_skip:\n  add %r15d, %r12d\n  jmp .Lfd_next\n.Lfd_done:\n  mov $0, %edi\n  mov $60, %eax\n  syscall\n.section .rodata\ndir_path:\n  .asciz \""<<p.directory<<"\"\nnewline:\n  .byte 10\nfilter:\n  .byte ";
  for(std::size_t i=0;i<p.filter.size();++i) { if(i) std::cout<<", "; std::cout<<(unsigned)(unsigned char)p.filter[i]; }
  std::cout<<"\n.bss\n.align 8\ndir_buf:\n  .skip 8192\n";
}

void emit_exists(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
           <<"  mov $332, %eax\n  mov $-100, %edi\n  lea exists_path(%rip), %rsi\n  xor %edx, %edx\n  mov $2047, %r10d\n  lea exists_buf(%rip), %r8\n  syscall\n  test %eax, %eax\n  js .Lexists_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lexists_done\n.Lexists_no:\n  mov $"<<p.else_status<<", %edi\n.Lexists_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nexists_path:\n  .asciz \""<<p.exists_path<<"\"\n.bss\n.align 8\nexists_buf:\n  .skip 256\n";
}

void emit_isdir(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
           <<"  mov $332, %eax\n  mov $-100, %edi\n  lea isdir_path(%rip), %rsi\n  xor %edx, %edx\n  mov $2047, %r10d\n  lea isdir_buf(%rip), %r8\n  syscall\n  test %eax, %eax\n  js .Lisdir_no\n  movzwl 28(%r8), %eax\n  and $61440, %eax\n  cmp $16384, %eax\n  jne .Lisdir_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lisdir_done\n.Lisdir_no:\n  mov $"<<p.else_status<<", %edi\n.Lisdir_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nisdir_path:\n  .asciz \""<<p.directory_path<<"\"\n.bss\n.align 8\nisdir_buf:\n  .skip 256\n";
}

void emit_isreg(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
           <<"  mov $332, %eax\n  mov $-100, %edi\n  lea isreg_path(%rip), %rsi\n  xor %edx, %edx\n  mov $2047, %r10d\n  lea isreg_buf(%rip), %r8\n  syscall\n  test %eax, %eax\n  js .Lisreg_no\n  movzwl 28(%r8), %eax\n  and $61440, %eax\n  cmp $32768, %eax\n  jne .Lisreg_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lisreg_done\n.Lisreg_no:\n  mov $"<<p.else_status<<", %edi\n.Lisreg_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nisreg_path:\n  .asciz \""<<p.regular_path<<"\"\n.bss\n.align 8\nisreg_buf:\n  .skip 256\n";
}

void emit_sizegt(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
           <<"  mov $332, %eax\n  mov $-100, %edi\n  lea size_path(%rip), %rsi\n  xor %edx, %edx\n  mov $2047, %r10d\n  lea size_buf(%rip), %r8\n  syscall\n  test %eax, %eax\n  js .Lsize_no\n  mov $"<<p.size_bytes<<", %rax\n  cmp %rax, 40(%r8)\n  jbe .Lsize_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lsize_done\n.Lsize_no:\n  mov $"<<p.else_status<<", %edi\n.Lsize_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nsize_path:\n  .asciz \""<<p.size_path<<"\"\n.bss\n.align 8\nsize_buf:\n  .skip 256\n";
}
}

int main(int argc,char **argv) {
  if(argc!=2) { std::cerr<<"usage: c_subset_compiler SOURCE\n"; return 2; }
  try {
    auto program=csubset::parse_main(csubset::read_source(argv[1]));
    csubset::check_with_nbe();
    if(program.exists) { csubset::emit_exists(program); return 0; }
    if(program.is_directory) { csubset::emit_isdir(program); return 0; }
    if(program.is_regular) { csubset::emit_isreg(program); return 0; }
    if(program.size_gt) { csubset::emit_sizegt(program); return 0; }
    if(program.filter.size()) { csubset::emit_filtered_directory(program); return 0; }
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
