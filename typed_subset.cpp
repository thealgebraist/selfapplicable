#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// A conventional, deliberately typed C-like subset.  This file builds an AST
// directly (the parser is intentionally separate from the typed core), checks
// it, folds static expressions, and emits a documented AArch64 ABI subset.
namespace tc {
struct Type { enum Tag { Int, Bool, String, Array, Void } tag; std::size_t length=0; friend bool operator==(Type,Type)=default; };
constexpr Type I{Type::Int}, B{Type::Bool}, S{Type::String}, V{Type::Void};
Type array(std::size_t n){return {Type::Array,n};}
struct Expr; using E=std::shared_ptr<const Expr>;
struct Int { std::int64_t n; }; struct Bool { bool b; }; struct Str { std::string s; };
struct Var { std::string n; }; struct ArrayLit { std::vector<E> xs; };
struct Index { E a,i; }; struct If { E c,t,f; };
struct Bin { enum Op { Add,Sub,Mul,Eq,And,Or } op; E l,r; };
struct Call { std::string f; std::vector<E> args; };
struct Expr { using Node=std::variant<Int,Bool,Str,Var,ArrayLit,Index,If,Bin,Call>; Node n; template<class X> Expr(X x):n(std::move(x)){} };
template<class X,class... A>E e(A&&...a){return std::make_shared<const Expr>(X{std::forward<A>(a)...});}
E integer(std::int64_t n){return e<Int>(n);} E boolean(bool b){return e<Bool>(b);} E string(std::string s){return e<Str>(std::move(s));} E var(std::string n){return e<Var>(std::move(n));}
E array_lit(std::vector<E> x){return e<ArrayLit>(std::move(x));} E index(E a,E i){return e<Index>(a,i);} E iff(E c,E t,E f){return e<If>(c,t,f);}
E bin(Bin::Op o,E l,E r){return e<Bin>(o,std::move(l),std::move(r));} E call(std::string f,std::vector<E>a){return e<Call>(std::move(f),std::move(a));}
struct Let {std::string n;Type t;E x; }; struct Return {E x;}; struct ExprStmt {E x;}; using Stmt=std::variant<Let,Return,ExprStmt>;
struct Function {std::string name; std::vector<std::pair<std::string,Type>> params; Type result; std::vector<Stmt> body;};
struct Program {std::vector<Function> functions;};
using Env=std::map<std::string,Type>; using FEnv=std::map<std::string,Function const*>;
Type infer(const E& x,const Env& env,const FEnv& fs);
void require(Type got,Type want){if(!(got==want))throw std::runtime_error("type mismatch");}
Type infer(const E& x,const Env& env,const FEnv& fs){return std::visit([&](auto const& n)->Type{using N=std::decay_t<decltype(n)>;
 if constexpr(std::is_same_v<N,Int>)return I; else if constexpr(std::is_same_v<N,Bool>)return B; else if constexpr(std::is_same_v<N,Str>)return S;
 else if constexpr(std::is_same_v<N,Var>){auto i=env.find(n.n);if(i==env.end())throw std::runtime_error("unbound variable");return i->second;}
 else if constexpr(std::is_same_v<N,ArrayLit>){if(n.xs.empty())return array(0);auto t=infer(n.xs[0],env,fs);for(auto const& y:n.xs)require(infer(y,env,fs),t);return array(n.xs.size());}
 else if constexpr(std::is_same_v<N,Index>){auto a=infer(n.a,env,fs);if(a.tag!=Type::Array)throw std::runtime_error("indexing non-array");require(infer(n.i,env,fs),I);return I;}
 else if constexpr(std::is_same_v<N,If>){require(infer(n.c,env,fs),B);auto t=infer(n.t,env,fs);require(infer(n.f,env,fs),t);return t;}
 else if constexpr(std::is_same_v<N,Bin>){auto l=infer(n.l,env,fs),r=infer(n.r,env,fs);if(n.op==Bin::Eq||n.op==Bin::And||n.op==Bin::Or){require(l,n.op==Bin::Eq?r:B);require(r,l);return B;}require(l,I);require(r,I);return I;}
 else {auto f=fs.find(n.f);if(f==fs.end())throw std::runtime_error("unknown function");if(f->second->params.size()!=n.args.size())throw std::runtime_error("arity");for(std::size_t i=0;i<n.args.size();++i)require(infer(n.args[i],env,fs),f->second->params[i].second);return f->second->result;}
},x->n);}
void check(const Function& f,const FEnv& fs){Env e;for(auto const& p:f.params)e[p.first]=p.second;bool returned=false;for(auto const& s:f.body)std::visit([&](auto const& n){using N=std::decay_t<decltype(n)>;if constexpr(std::is_same_v<N,Let>){require(infer(n.x,e,fs),n.t);e[n.n]=n.t;}else if constexpr(std::is_same_v<N,Return>){require(infer(n.x,e,fs),f.result);returned=true;}else (void)infer(n.x,e,fs);},s);if(!returned&&f.result.tag!=Type::Void)throw std::runtime_error("missing return");}

E fold(const E& x,const Env& env){return std::visit([&](auto const& n)->E{using N=std::decay_t<decltype(n)>;
 if constexpr(std::is_same_v<N,Int>||std::is_same_v<N,Bool>||std::is_same_v<N,Str>)return x;
 else if constexpr(std::is_same_v<N,Var>){return x;}
 else if constexpr(std::is_same_v<N,If>){auto c=fold(n.c,env);if(auto b=std::get_if<Bool>(&c->n))return fold(b->b?n.t:n.f,env);return e<If>(c,fold(n.t,env),fold(n.f,env));}
 else if constexpr(std::is_same_v<N,Bin>){auto l=fold(n.l,env),r=fold(n.r,env);auto a=std::get_if<Int>(&l->n),b=std::get_if<Int>(&r->n);if(a&&b){if(n.op==Bin::Add)return integer(a->n+b->n);if(n.op==Bin::Sub)return integer(a->n-b->n);if(n.op==Bin::Mul)return integer(a->n*b->n);}return e<Bin>(n.op,l,r);}
 else return x;
},x->n);}

struct Emitter {
  std::vector<std::string> out, data;
  std::map<std::string,std::string> regs; int next=0, label=0;
  void p(std::string x){out.push_back("  "+x);}
  void expr(const E& x){std::visit([&](auto const& n){using N=std::decay_t<decltype(n)>;
    if constexpr(std::is_same_v<N,Int>) p("mov x0, #"+std::to_string(n.n));
    else if constexpr(std::is_same_v<N,Bool>) p(std::string("mov x0, #")+(n.b?"1":"0"));
    else if constexpr(std::is_same_v<N,Str>){auto l=".LC"+std::to_string(data.size());data.push_back(l+": .asciz \""+n.s+"\"");p("adrp x0, "+l);p("add x0, x0, :lo12:"+l);}
    else if constexpr(std::is_same_v<N,Var>) p("mov x0, "+regs.at(n.n));
    else if constexpr(std::is_same_v<N,ArrayLit>){auto l=".LA"+std::to_string(data.size());std::string d=l+":";for(auto const& a:n.xs)if(auto i=std::get_if<Int>(&a->n))d+="\n  .quad "+std::to_string(i->n);data.push_back(d);p("adrp x0, "+l);p("add x0, x0, :lo12:"+l);}
    else if constexpr(std::is_same_v<N,Index>){expr(n.a);p("str x0, [sp, #-16]!");expr(n.i);p("lsl x0, x0, #3");p("ldr x1, [sp], #16");p("ldr x0, [x1, x0]");}
    else if constexpr(std::is_same_v<N,If>){auto k=label++;expr(n.c);p("cmp x0, #0");p("b.eq .Lelse"+std::to_string(k));expr(n.t);p("b .Lend"+std::to_string(k));p(".Lelse"+std::to_string(k)+":");expr(n.f);p(".Lend"+std::to_string(k)+":");}
    else if constexpr(std::is_same_v<N,Bin>){expr(n.l);p("str x0, [sp, #-16]!");expr(n.r);p("mov x1, x0");p("ldr x0, [sp], #16");if(n.op==Bin::Add)p("add x0, x0, x1");else if(n.op==Bin::Sub)p("sub x0, x0, x1");else if(n.op==Bin::Mul)p("mul x0, x0, x1");else if(n.op==Bin::Eq){p("cmp x0, x1");p("cset x0, eq");}}
    else if constexpr(std::is_same_v<N,Call>){int i=0;for(auto const& a:n.args){expr(a);p("mov x"+std::to_string(i++)+", x0");}p("bl "+n.f);}
  },x->n);}
 std::string function(const Function& f){out={f.name+":","  stp x29, x30, [sp, #-16]!","  mov x29, sp"};int i=0;for(auto const& a:f.params)regs[a.first]="x"+std::to_string(i++);int local=2;for(auto const& s:f.body){if(auto z=std::get_if<Let>(&s)){expr(fold(z->x,{}));regs[z->n]="x"+std::to_string(local);p("mov "+regs[z->n]+", x0");++local;}else if(auto z=std::get_if<ExprStmt>(&s)){expr(fold(z->x,{}));}else if(auto r=std::get_if<Return>(&s)){expr(fold(r->x,{}));p("ldp x29, x30, [sp], #16");p("ret");}}out.push_back("");std::string z;for(auto const& x:out)z+=x+"\n";return z;}
  std::string syscall_write(){return "sys_write:\n  mov x8, #64\n  svc #0\n  ret\n";}
};
}
int main(){using namespace tc;try{Function add{"add",{{"a",I},{"b",I}},I,{Return{bin(Bin::Add,var("a"),var("b"))}}};Function builtin{"sys_write",{{"fd",I},{"buf",S},{"len",I}},I,{Return{integer(0)}}};Function mainf{"main",{},I,{Let{"arr",array(2),array_lit({integer(7),integer(9)})},Let{"picked",I,index(var("arr"),integer(1))},Let{"ok",B,bin(Bin::Eq,integer(2),integer(2))},ExprStmt{call("sys_write",{integer(1),string("ok\\n"),integer(3)})},Return{iff(var("ok"),call("add",{integer(20),integer(22)}),integer(0))}}};Program p{{add,mainf}};FEnv fs;for(auto const& f:p.functions)fs[f.name]=&f;fs[builtin.name]=&builtin;for(auto const& f:p.functions)check(f,fs);Emitter gen;std::cout<<".text\n.global main\n";for(auto const& f:p.functions)std::cout<<gen.function(f);std::cout<<gen.syscall_write()<<".section .rodata\n";for(auto const& d:gen.data)std::cout<<d<<"\n";std::cout<<"typecheck: PASS\n";}catch(std::exception const& e){std::cerr<<"error: "<<e.what()<<'\n';return 1;}}
