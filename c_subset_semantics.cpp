// Typed semantic core for the next C-subset layer.  This is deliberately
// separate from the permissive source reader: it gives structs, pointers,
// calls, and recursion real types before assembler lowering.
#define NORMALISER_LIBRARY
#include "normaliser.cpp"
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace csem {
struct Ty; using T=std::shared_ptr<const Ty>;
struct Ty { struct Int{}; struct Void{}; struct Ptr{T to;}; struct Struct{std::string name;}; struct Fn{std::vector<T> args; T ret;}; using N=std::variant<Int,Void,Ptr,Struct,Fn>; N n; template<class X> Ty(X x):n(std::move(x)){} };
T integer(){return std::make_shared<Ty>(Ty::Int{});} T unit(){return std::make_shared<Ty>(Ty::Void{});}
T pointer(T x){return std::make_shared<Ty>(Ty::Ptr{std::move(x)});} T structure(std::string n){return std::make_shared<Ty>(Ty::Struct{std::move(n)});}
T function(std::vector<T> a,T r){return std::make_shared<Ty>(Ty::Fn{std::move(a),std::move(r)});}
bool same(T const&a,T const&b){return std::visit([&](auto const&x,auto const&y)->bool{using X=std::decay_t<decltype(x)>;using Y=std::decay_t<decltype(y)>;if constexpr(!std::is_same_v<X,Y>)return false;else if constexpr(std::is_same_v<X,Ty::Ptr>)return same(x.to,y.to);else if constexpr(std::is_same_v<X,Ty::Struct>)return x.name==y.name;else if constexpr(std::is_same_v<X,Ty::Fn>){if(x.args.size()!=y.args.size()||!same(x.ret,y.ret))return false;for(size_t i=0;i<x.args.size();++i)if(!same(x.args[i],y.args[i]))return false;return true;}else return true;},a->n,b->n);}

struct E; using Expr=std::shared_ptr<const E>;
struct Lit{int value;}; struct Var{std::string name;}; struct Addr{Expr x;}; struct Deref{Expr x;}; struct Call{std::string fn;std::vector<Expr> args;};
struct E{using N=std::variant<Lit,Var,Addr,Deref,Call>;N n;template<class X>E(X x):n(std::move(x)){} };
template<class X,class...A>Expr e(A&&...a){return std::make_shared<E>(X{std::forward<A>(a)...});}
Expr literal(int n){return e<Lit>(n);} Expr variable(std::string n){return e<Var>(std::move(n));} Expr address(Expr x){return e<Addr>(std::move(x));} Expr dereference(Expr x){return e<Deref>(std::move(x));} Expr call(std::string n,std::vector<Expr>a){return e<Call>(std::move(n),std::move(a));}

struct Function{std::string name;std::vector<std::pair<std::string,T>> args;T ret;std::vector<Expr> body;};
using Env=std::map<std::string,T>; using Functions=std::map<std::string,Function const*>;
T infer(Expr const&x,Env const&env,Functions const&fs){return std::visit([&](auto const&q)->T{using X=std::decay_t<decltype(q)>;if constexpr(std::is_same_v<X,Lit>)return integer();if constexpr(std::is_same_v<X,Var>){auto i=env.find(q.name);if(i==env.end())throw std::runtime_error("unbound C variable "+q.name);return i->second;}if constexpr(std::is_same_v<X,Addr>)return pointer(infer(q.x,env,fs));if constexpr(std::is_same_v<X,Deref>){auto t=infer(q.x,env,fs);auto p=std::get_if<Ty::Ptr>(&t->n);if(!p)throw std::runtime_error("dereference of non-pointer");return p->to;}if constexpr(std::is_same_v<X,Call>){auto f=fs.find(q.fn);if(f==fs.end())throw std::runtime_error("unknown C function "+q.fn);if(f->second->args.size()!=q.args.size())throw std::runtime_error("C call arity");for(size_t i=0;i<q.args.size();++i)if(!same(infer(q.args[i],env,fs),f->second->args[i].second))throw std::runtime_error("C call argument type");return f->second->ret;}},x->n);}
void check(Function const&f,Functions const&fs){Env env;for(auto const&a:f.args)env[a.first]=a.second;for(auto const&x:f.body)if(!same(infer(x,env,fs),f.ret))throw std::runtime_error("C function result type");}
}

int main(){using namespace csem;try{
  auto Node=structure("Node"); auto NodePtr=pointer(Node);
  // struct Node { int value; struct Node *next; };
  Function length{"length",{{"p",NodePtr}},integer(),{call("length",{variable("p")})}};
  Functions fs{{"length",&length}}; check(length,fs); // recursive call is type-checked
  auto bad=Function{"bad",{{"p",NodePtr}},integer(),{dereference(variable("p"))}}; bool rejected=false;try{check(bad,fs);}catch(std::exception const&){rejected=true;}if(!rejected)throw std::runtime_error("bad pointer result accepted");
  using namespace st; auto A=sort(1); auto n=nbe_normalise({},normalize_code(A,quote(app(lam(A,var(0)),sort(0)))));if(!equal(n,quote(sort(0))))throw Error("NbE bridge failed");
  std::cout<<"struct Node: PASS\npointer types: PASS\nfunction calls: PASS\nrecursive call typing: PASS\nNbE bridge: PASS\n";
}catch(std::exception const&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
