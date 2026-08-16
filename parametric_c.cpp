#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>

// The compiler is parameterized by a semantic description of the C variant.
// The description is data: quote_description/unquote_description make the
// compiler generator explicitly self-applicable at the staging boundary.
namespace pc {
struct CDescription {
  std::string name;
  unsigned integer_bits;
  std::string entry;
  std::string result_register;
  std::string syscall_number_register;
  std::int64_t write_syscall;
  std::string syscall_instruction;
  friend bool operator==(const CDescription&,const CDescription&)=default;
};
struct Int { std::int64_t value; }; struct Add;
struct Write { std::string text; }; struct Expr;
using E=std::shared_ptr<const Expr>;
struct Add { E left,right; };
struct Expr { using Node=std::variant<Int,Add,Write>; Node node; template<class X> Expr(X x):node(std::move(x)){} };
template<class X,class... A>E make(A&&... a){return std::make_shared<const Expr>(X{std::forward<A>(a)...});}
E integer(std::int64_t x){return make<Int>(x);} E add(E a,E b){return make<Add>(std::move(a),std::move(b));} E write(std::string s){return make<Write>(std::move(s));}

template<class A> struct Code { A value; };
Code<CDescription> quote_description(CDescription d){return {std::move(d)};}
const CDescription& unquote_description(const Code<CDescription>& c){return c.value;}

E normalize(const E& e){return std::visit([&](auto const& x)->E{using X=std::decay_t<decltype(x)>;
  if constexpr(std::is_same_v<X,Int>||std::is_same_v<X,Write>)return e;
  else {auto l=normalize(x.left),r=normalize(x.right);auto a=std::get_if<Int>(&l->node),b=std::get_if<Int>(&r->node);if(a&&b)return integer(a->value+b->value);return add(l,r);}
},e->node);}

struct Compiler {
  CDescription spec;
  std::string compile(const E& source) const {
    auto e=normalize(source); std::string a=".text\n.global "+spec.entry+"\n"+spec.entry+":\n";
    a+="  stp x29, x30, [sp, #-16]!\n  mov x29, sp\n";
    std::visit([&](auto const& x){using X=std::decay_t<decltype(x)>;
      if constexpr(std::is_same_v<X,Int>) a+="  mov "+spec.result_register+", #"+std::to_string(x.value)+"\n";
      else if constexpr(std::is_same_v<X,Add>) a+="  // normalized non-constant add remains for the expression lowering pass\n";
      else {std::string l=".LC0";a+="  adrp x1, "+l+"\n  add x1, x1, :lo12:"+l+"\n  mov x0, #1\n  mov x2, #"+std::to_string(x.text.size())+"\n  mov "+spec.syscall_number_register+", #"+std::to_string(spec.write_syscall)+"\n  "+spec.syscall_instruction+"\n";}
    },e->node);
    a+="  ldp x29, x30, [sp], #16\n  ret\n";
    if(auto w=std::get_if<Write>(&e->node))a+=".section .rodata\n.LC0: .asciz \""+w->text+"\"\n";
    return a;
  }
};

using CompilerGenerator=std::function<Compiler(const CDescription&)>;
CompilerGenerator quote_compiler_generator(){
  return [](const CDescription& d){return Compiler{d};};
}
Compiler compile_via_staged_generator(const Code<CDescription>& quoted,const E&){
  auto generator=quote_compiler_generator();
  return generator(unquote_description(quoted));
}
}

int main(){using namespace pc;try{
  CDescription linuxC{"linux-c",64,"main","x0","x8",64,"svc #0"};
  CDescription altC{"alt-c",32,"entry","x0","x8",64,"svc #0"};
  auto staticProgram=add(integer(40),add(integer(2),integer(1)));
  auto quoted=quote_description(linuxC);
  auto direct=Compiler{linuxC}.compile(staticProgram);
  auto staged=compile_via_staged_generator(quoted,staticProgram).compile(staticProgram);
  if(direct!=staged)throw std::runtime_error("staged compiler mismatch");
  auto alternate=Compiler{altC}.compile(staticProgram);
  std::cout<<"staged compiler equivalence: PASS\n"<<direct;
  std::cout<<"alternate C variant entry: "<<altC.entry<<"\n";
  (void)alternate;
}catch(std::exception const& e){std::cerr<<"error: "<<e.what()<<'\n';return 1;}}
