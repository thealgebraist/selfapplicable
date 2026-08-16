#include <cctype>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// A compact Python-like compiler.  The source language is intentionally small:
//   let name = expression; ... return expression
// Expressions contain integers, names, +, -, and *.
namespace pyarm {
struct Expr; using E=std::shared_ptr<const Expr>;
struct Lit{std::int64_t n;}; struct Name{std::string s;};
struct Bin{char op; E l,r;};
struct Expr{using Node=std::variant<Lit,Name,Bin>; Node n;
  template<class X> Expr(X x):n(std::move(x)){} };
E lit(std::int64_t n){return std::make_shared<const Expr>(Lit{n});}
E name(std::string s){return std::make_shared<const Expr>(Name{std::move(s)});}
E bin(char op,E l,E r){return std::make_shared<const Expr>(Bin{op,std::move(l),std::move(r)});}
struct Let{std::string s;E e;}; struct Return{E e;}; using Stmt=std::variant<Let,Return>;
struct Program{std::vector<Stmt> stmts;};

struct Lexer{std::string s;std::size_t p=0;void ws(){while(p<s.size()&&std::isspace((unsigned char)s[p]))++p;}
  bool eat(char c){ws();if(p<s.size()&&s[p]==c){++p;return true;}return false;}
  std::string ident(){ws();std::size_t b=p;while(p<s.size()&&(std::isalnum((unsigned char)s[p])||s[p]=='_'))++p;if(b==p)throw std::runtime_error("identifier expected");return s.substr(b,p-b);}
  std::int64_t integer(){ws();std::size_t b=p;while(p<s.size()&&std::isdigit((unsigned char)s[p]))++p;if(b==p)throw std::runtime_error("integer expected");return std::stoll(s.substr(b,p-b));}};
struct Parser{Lexer l;
  E atom(){l.ws();if(l.p<l.s.size()&&std::isdigit((unsigned char)l.s[l.p]))return lit(l.integer());if(l.eat('(')){auto x=expr();if(!l.eat(')'))throw std::runtime_error("missing )");return x;}return name(l.ident());}
  E product(){auto x=atom();while(true){l.ws();if(l.eat('*'))x=bin('*',x,atom());else return x;}}
  E expr(){auto x=product();while(true){l.ws();if(l.eat('+'))x=bin('+',x,product());else if(l.eat('-'))x=bin('-',x,product());else return x;}}
  Program program(){Program p;while(true){l.ws();if(l.p>=l.s.size())break;auto k=l.ident();if(k=="let"){auto n=l.ident();if(!l.eat('='))throw std::runtime_error("missing =");auto e=expr();if(!l.eat(';'))throw std::runtime_error("missing ;");p.stmts.emplace_back(Let{std::move(n),e});}else if(k=="return"){auto e=expr();l.eat(';');p.stmts.emplace_back(Return{e});break;}else throw std::runtime_error("expected let or return");}return p;}};

using ConstEnv=std::map<std::string,std::int64_t>;
E normalize(const E& e,const ConstEnv& env){return std::visit([&](auto const& x)->E{
  using X=std::decay_t<decltype(x)>;
  if constexpr(std::is_same_v<X,Lit>)return e;
  else if constexpr(std::is_same_v<X,Name>){auto i=env.find(x.s);return i==env.end()?e:lit(i->second);}
  else {
  auto a=normalize(x.l,env),b=normalize(x.r,env);auto la=std::get_if<Lit>(&a->n),lb=std::get_if<Lit>(&b->n);
  if(la&&lb){if(x.op=='+')return lit(la->n+lb->n);if(x.op=='-')return lit(la->n-lb->n);return lit(la->n*lb->n);}return bin(x.op,a,b);
  }
},e->n);}
Program normalize(const Program& p){Program q;ConstEnv env;for(auto const& s:p.stmts){if(auto z=std::get_if<Let>(&s)){auto e=normalize(z->e,env);if(auto v=std::get_if<Lit>(&e->n)){env[z->s]=v->n;continue;}q.stmts.emplace_back(Let{z->s,e});}else{auto e=normalize(std::get<Return>(s).e,env);q.stmts.emplace_back(Return{e});}}return q;}

struct Emitter{std::vector<std::string> out;std::map<std::string,int> slots;int next=0;
  void line(std::string s){out.push_back("  "+std::move(s));}
  void expr(const E& e){std::visit([&](auto const& x){using X=std::decay_t<decltype(x)>;
    if constexpr(std::is_same_v<X,Lit>)line("mov x0, #"+std::to_string(x.n));
    else if constexpr(std::is_same_v<X,Name>){auto i=slots.find(x.s);if(i==slots.end())throw std::runtime_error("unbound name");line("ldr x0, [x29, #-"+std::to_string(i->second)+"]");}
    else {expr(x.l);line("str x0, [sp, #-16]!");expr(x.r);line("mov x1, x0");line("ldr x0, [sp], #16");if(x.op=='+')line("add x0, x0, x1");else if(x.op=='-')line("sub x0, x0, x1");else line("mul x0, x0, x1");}
  },e->n);}
  std::string program(const Program& p){out={".text",".global main","main:","  stp x29, x30, [sp, #-16]!","  mov x29, sp"};for(auto const& s:p.stmts){if(auto z=std::get_if<Let>(&s)){expr(z->e);int off=++next*16;slots[z->s]=off;line("str x0, [x29, #-"+std::to_string(off)+"]");}else expr(std::get<Return>(s).e);}out.push_back("  ldp x29, x30, [sp], #16");out.push_back("  ret");std::string r;for(auto const& x:out)r+=x+"\n";return r;}
};
std::string show(const E& e){return std::visit([&](auto const& x)->std::string{using X=std::decay_t<decltype(x)>;if constexpr(std::is_same_v<X,Lit>)return std::to_string(x.n);else if constexpr(std::is_same_v<X,Name>)return x.s;else return "("+show(x.l)+x.op+show(x.r)+")";},e->n);}
}
int main(){using namespace pyarm;try{std::string source="let x = 2 + 3 * 4; let y = x * 10; return y + 1;";auto p=Parser{Lexer{source}}.program();auto n=normalize(p);auto& ret=std::get<Return>(n.stmts.back());std::cout<<"normalized return: "<<show(ret.e)<<"\n";std::cout<<Emitter{}.program(n);return 0;}catch(std::exception const& e){std::cerr<<"error: "<<e.what()<<'\n';return 1;}}
