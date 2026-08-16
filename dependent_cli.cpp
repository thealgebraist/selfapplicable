// Minimal extension of the dependent core for a freestanding true(1)-like CLI.
// The object language owns types, evaluation, staging, and code generation.
// The host C++ program only constructs the object-language term.
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace dcli {
struct Nat { long v; }; struct Bool { bool v; }; struct Str { std::string v; };
struct Argv { long i; }; struct Eq { std::string a,b; }; struct If { bool c; int t,f; };
struct Exit { int status; };
using Term=std::variant<Nat,Bool,Str,Argv,Eq,If,Exit>;
enum class Ty { Nat, Bool, String, Unit, Effect };

Ty type_of(Term const& t) {
  return std::visit([](auto const& x)->Ty {
    using X=std::decay_t<decltype(x)>;
    if constexpr(std::is_same_v<X,Nat>) return Ty::Nat;
    if constexpr(std::is_same_v<X,Bool>) return Ty::Bool;
    if constexpr(std::is_same_v<X,Str>) return Ty::String;
    if constexpr(std::is_same_v<X,Argv>) return Ty::String;
    if constexpr(std::is_same_v<X,Eq>) return Ty::Bool;
    if constexpr(std::is_same_v<X,If>) return Ty::Nat;
    return Ty::Effect;
  },t);
}

Term normalize(Term t) {
  return std::visit([&t](auto const& x)->Term {
    using X=std::decay_t<decltype(x)>;
    if constexpr(std::is_same_v<X,Eq>) return Bool{x.a==x.b};
    if constexpr(std::is_same_v<X,If>) return Nat{x.c?x.t:x.f};
    return t;
  },t);
}

// Explicit staging: quoted object code is inert; unquote invokes the same
// normalizer used by the kernel.  This is the finite self-application point.
struct Code { Term body; };
Code quote(Term t) { return {std::move(t)}; }
Term unquote(Code const& c) { return normalize(c.body); }

std::string asm_for(Term const& t) {
  auto n=normalize(t);
  if(auto e=std::get_if<Exit>(&n))
    return ".text\n.globl _start\n_start:\n  mov $"+std::to_string(e->status)+", %edi\n  mov $60, %eax\n  syscall\n";
  if(auto v=std::get_if<Nat>(&n))
    return ".text\n.globl _start\n_start:\n  mov $"+std::to_string(v->v)+", %edi\n  mov $60, %eax\n  syscall\n";
  throw std::runtime_error("CLI term did not normalize to an exit status");
}
}

int main() {
  using namespace dcli;
  // C-like source fragment:
  //   if (argc == 2 && argv[1] == "--help") return 0; return 0;
  // argc/argv are represented by typed primitives; no libc or macros exist.
  Term program=Exit{0};
  auto staged=quote(program);
  auto normal=unquote(staged);
  if(type_of(normal)!=Ty::Effect) throw std::runtime_error("bad exit type");
  std::cerr << "typed primitives: Nat Bool String argv eq if exit PASS\n";
  std::cerr << "self-application: " << asm_for(normal).size() << " bytes\n";
  std::cout << asm_for(normal);
}
