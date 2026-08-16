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
#include <utility>

namespace csem {
struct Ty; using T=std::shared_ptr<const Ty>;
struct Ty { struct Int{}; struct Void{}; struct Ptr{T to;}; struct Struct{std::string name;}; struct Fn{std::vector<T> args; T ret;}; using N=std::variant<Int,Void,Ptr,Struct,Fn>; N n; template<class X> Ty(X x):n(std::move(x)){} };
T integer(){return std::make_shared<Ty>(Ty::Int{});} T unit(){return std::make_shared<Ty>(Ty::Void{});}
T pointer(T x){return std::make_shared<Ty>(Ty::Ptr{std::move(x)});} T structure(std::string n){return std::make_shared<Ty>(Ty::Struct{std::move(n)});}
T function(std::vector<T> a,T r){return std::make_shared<Ty>(Ty::Fn{std::move(a),std::move(r)});}
bool same(T const&a,T const&b){return std::visit([&](auto const&x,auto const&y)->bool{using X=std::decay_t<decltype(x)>;using Y=std::decay_t<decltype(y)>;if constexpr(!std::is_same_v<X,Y>)return false;else if constexpr(std::is_same_v<X,Ty::Ptr>)return same(x.to,y.to);else if constexpr(std::is_same_v<X,Ty::Struct>)return x.name==y.name;else if constexpr(std::is_same_v<X,Ty::Fn>){if(x.args.size()!=y.args.size()||!same(x.ret,y.ret))return false;for(size_t i=0;i<x.args.size();++i)if(!same(x.args[i],y.args[i]))return false;return true;}else return true;},a->n,b->n);}

struct E; using Expr=std::shared_ptr<const E>;
struct Lit{int value;}; struct Var{std::string name;}; struct Addr{Expr x;}; struct Deref{Expr x;};
struct Member{Expr base;std::string field;}; struct Arrow{Expr base;std::string field;};
enum class BinOp { Add, Equal };
struct Binary{BinOp op;Expr left;Expr right;}; struct Assign{Expr left;Expr right;};
struct If{Expr condition;Expr yes;Expr no;};
struct While{Expr condition;Expr body;}; struct Sequence{std::vector<Expr> items;};
struct Index{Expr base;Expr index;};
struct Sizeof{T type;};
struct Alloc{T type;}; struct Free{Expr value;};
struct Call{std::string fn;std::vector<Expr> args;}; struct FunctionRef{std::string fn;}; struct IndirectCall{Expr callee;std::vector<Expr> args;};
struct E{using N=std::variant<Lit,Var,Addr,Deref,Member,Arrow,Binary,Assign,If,While,Sequence,Index,Sizeof,Alloc,Free,Call,FunctionRef,IndirectCall>;N n;template<class X>E(X x):n(std::move(x)){} };
template<class X,class...A>Expr e(A&&...a){return std::make_shared<E>(X{std::forward<A>(a)...});}
Expr literal(int n){return e<Lit>(n);} Expr variable(std::string n){return e<Var>(std::move(n));} Expr address(Expr x){return e<Addr>(std::move(x));} Expr dereference(Expr x){return e<Deref>(std::move(x));}
Expr member(Expr x,std::string n){return e<Member>(std::move(x),std::move(n));} Expr arrow(Expr x,std::string n){return e<Arrow>(std::move(x),std::move(n));}
Expr binary(BinOp op,Expr x,Expr y){return e<Binary>(op,std::move(x),std::move(y));} Expr assign(Expr x,Expr y){return e<Assign>(std::move(x),std::move(y));}
Expr conditional(Expr c,Expr y,Expr n){return e<If>(std::move(c),std::move(y),std::move(n));}
Expr while_loop(Expr c,Expr b){return e<While>(std::move(c),std::move(b));} Expr sequence(std::vector<Expr> xs){return e<Sequence>(std::move(xs));}
Expr index(Expr b,Expr i){return e<Index>(std::move(b),std::move(i));}
Expr sizeof_type(T t){return e<Sizeof>(std::move(t));}
Expr allocate(T t){return e<Alloc>(std::move(t));} Expr release(Expr x){return e<Free>(std::move(x));}
Expr call(std::string n,std::vector<Expr>a){return e<Call>(std::move(n),std::move(a));} Expr function_ref(std::string n){return e<FunctionRef>(std::move(n));} Expr indirect_call(Expr f,std::vector<Expr>a){return e<IndirectCall>(std::move(f),std::move(a));}

struct Function{std::string name;std::vector<std::pair<std::string,T>> args;T ret;std::vector<Expr> body;};
using Env=std::map<std::string,T>; using Functions=std::map<std::string,Function const*>;
using StructFields=std::map<std::string,std::map<std::string,T>>;
T field(T const&base,std::string const&name,StructFields const&ss){auto s=std::get_if<Ty::Struct>(&base->n);if(!s)throw std::runtime_error("field access on non-struct");auto si=ss.find(s->name);if(si==ss.end())throw std::runtime_error("unknown struct "+s->name);auto fi=si->second.find(name);if(fi==si->second.end())throw std::runtime_error("unknown field "+name);return fi->second;}
size_t size_of(T const&t,StructFields const&ss){return std::visit([&](auto const&q)->size_t{using X=std::decay_t<decltype(q)>;if constexpr(std::is_same_v<X,Ty::Int>)return 4;if constexpr(std::is_same_v<X,Ty::Void>)throw std::runtime_error("sizeof void");if constexpr(std::is_same_v<X,Ty::Ptr>)return 8;if constexpr(std::is_same_v<X,Ty::Fn>)return 8;if constexpr(std::is_same_v<X,Ty::Struct>){auto i=ss.find(q.name);if(i==ss.end())throw std::runtime_error("sizeof unknown struct "+q.name);size_t n=0;for(auto const&f:i->second)n+=size_of(f.second,ss);return n;}},t->n);}
bool lvalue(Expr const&x){return std::holds_alternative<Var>(x->n)||std::holds_alternative<Deref>(x->n)||std::holds_alternative<Member>(x->n)||std::holds_alternative<Arrow>(x->n);}
T infer(Expr const&x,Env const&env,Functions const&fs,StructFields const&ss){return std::visit([&](auto const&q)->T{using X=std::decay_t<decltype(q)>;if constexpr(std::is_same_v<X,Lit>)return integer();if constexpr(std::is_same_v<X,Var>){auto i=env.find(q.name);if(i==env.end())throw std::runtime_error("unbound C variable "+q.name);return i->second;}if constexpr(std::is_same_v<X,Addr>){if(!lvalue(q.x))throw std::runtime_error("address of non-lvalue");return pointer(infer(q.x,env,fs,ss));}if constexpr(std::is_same_v<X,Deref>){auto t=infer(q.x,env,fs,ss);auto p=std::get_if<Ty::Ptr>(&t->n);if(!p)throw std::runtime_error("dereference of non-pointer");return p->to;}if constexpr(std::is_same_v<X,Member>)return field(infer(q.base,env,fs,ss),q.field,ss);if constexpr(std::is_same_v<X,Arrow>){auto t=infer(q.base,env,fs,ss);auto p=std::get_if<Ty::Ptr>(&t->n);if(!p)throw std::runtime_error("arrow access on non-pointer");return field(p->to,q.field,ss);}if constexpr(std::is_same_v<X,Binary>){auto l=infer(q.left,env,fs,ss);auto r=infer(q.right,env,fs,ss);if(q.op==BinOp::Add){if(same(l,integer())&&same(r,integer()))return integer();if(auto p=std::get_if<Ty::Ptr>(&l->n);p&&same(r,integer()))return l;if(auto p=std::get_if<Ty::Ptr>(&r->n);p&&same(l,integer()))return r;throw std::runtime_error("pointer addition types");}if(!same(l,r))throw std::runtime_error("equality operand types");return integer();}if constexpr(std::is_same_v<X,Assign>){if(!lvalue(q.left))throw std::runtime_error("assignment to non-lvalue");auto l=infer(q.left,env,fs,ss);auto r=infer(q.right,env,fs,ss);if(!same(l,r))throw std::runtime_error("assignment type mismatch");return l;}if constexpr(std::is_same_v<X,If>){if(!same(infer(q.condition,env,fs,ss),integer()))throw std::runtime_error("conditional requires integer condition");auto y=infer(q.yes,env,fs,ss);auto n=infer(q.no,env,fs,ss);if(!same(y,n))throw std::runtime_error("conditional branch type mismatch");return y;}if constexpr(std::is_same_v<X,While>){if(!same(infer(q.condition,env,fs,ss),integer()))throw std::runtime_error("while requires integer condition");(void)infer(q.body,env,fs,ss);return unit();}if constexpr(std::is_same_v<X,Sequence>){T result=unit();for(auto const&item:q.items)result=infer(item,env,fs,ss);return result;}if constexpr(std::is_same_v<X,Index>){auto b=infer(q.base,env,fs,ss);if(!same(infer(q.index,env,fs,ss),integer()))throw std::runtime_error("index is not integer");auto p=std::get_if<Ty::Ptr>(&b->n);if(!p)throw std::runtime_error("index of non-pointer");return p->to;}if constexpr(std::is_same_v<X,Sizeof>){(void)size_of(q.type,ss);return integer();}if constexpr(std::is_same_v<X,Alloc>){(void)size_of(q.type,ss);return pointer(q.type);}if constexpr(std::is_same_v<X,Free>){auto t=infer(q.value,env,fs,ss);if(!std::holds_alternative<Ty::Ptr>(t->n))throw std::runtime_error("free of non-pointer");return unit();}if constexpr(std::is_same_v<X,FunctionRef>){auto f=fs.find(q.fn);if(f==fs.end())throw std::runtime_error("unknown function reference");std::vector<T> args;for(auto const&a:f->second->args)args.push_back(a.second);return function(std::move(args),f->second->ret);}if constexpr(std::is_same_v<X,Call>){auto f=fs.find(q.fn);if(f==fs.end())throw std::runtime_error("unknown C function "+q.fn);if(f->second->args.size()!=q.args.size())throw std::runtime_error("C call arity");for(size_t i=0;i<q.args.size();++i)if(!same(infer(q.args[i],env,fs,ss),f->second->args[i].second))throw std::runtime_error("C call argument type");return f->second->ret;}if constexpr(std::is_same_v<X,IndirectCall>){auto ft=infer(q.callee,env,fs,ss);auto fn=std::get_if<Ty::Fn>(&ft->n);if(!fn)throw std::runtime_error("indirect call of non-function");if(fn->args.size()!=q.args.size())throw std::runtime_error("indirect call arity");for(size_t i=0;i<q.args.size();++i)if(!same(infer(q.args[i],env,fs,ss),fn->args[i]))throw std::runtime_error("indirect call argument type");return fn->ret;}},x->n);}
void check(Function const&f,Functions const&fs,StructFields const&ss){Env env;for(auto const&a:f.args)env[a.first]=a.second;for(auto const&x:f.body)if(!same(infer(x,env,fs,ss),f.ret))throw std::runtime_error("C function result type");}

struct ReturnStmt{Expr value;}; struct ExprStmt{Expr value;};
struct IfStmt{Expr condition;std::vector<struct Stmt> yes,no;};
struct WhileStmt{Expr condition;std::vector<struct Stmt> body;};
struct Stmt{using N=std::variant<ReturnStmt,ExprStmt,IfStmt,WhileStmt>;N n;template<class X>Stmt(X x):n(std::move(x)){} };
Stmt return_stmt(Expr x){return Stmt(ReturnStmt{std::move(x)});} Stmt expr_stmt(Expr x){return Stmt(ExprStmt{std::move(x)});}
Stmt if_stmt(Expr c,std::vector<Stmt>y,std::vector<Stmt>n){return Stmt(IfStmt{std::move(c),std::move(y),std::move(n)});} Stmt while_stmt(Expr c,std::vector<Stmt>b){return Stmt(WhileStmt{std::move(c),std::move(b)});}
void check_statements(std::vector<Stmt> const&body,T const&ret,Env const&env,Functions const&fs,StructFields const&ss){for(auto const&s:body)std::visit([&](auto const&q){using X=std::decay_t<decltype(q)>;if constexpr(std::is_same_v<X,ReturnStmt>){if(!same(infer(q.value,env,fs,ss),ret))throw std::runtime_error("return type mismatch");}if constexpr(std::is_same_v<X,ExprStmt>){(void)infer(q.value,env,fs,ss);}if constexpr(std::is_same_v<X,IfStmt>){if(!same(infer(q.condition,env,fs,ss),integer()))throw std::runtime_error("if requires integer condition");check_statements(q.yes,ret,env,fs,ss);check_statements(q.no,ret,env,fs,ss);}if constexpr(std::is_same_v<X,WhileStmt>){if(!same(infer(q.condition,env,fs,ss),integer()))throw std::runtime_error("while requires integer condition");check_statements(q.body,unit(),env,fs,ss);}},s.n);}
void check_body(Function const&f,std::vector<Stmt> const&body,Functions const&fs,StructFields const&ss){Env env;for(auto const&a:f.args)env[a.first]=a.second;check_statements(body,f.ret,env,fs,ss);}
}

int main(){using namespace csem;try{
  auto Node=structure("Node"); auto NodePtr=pointer(Node);
  StructFields structs{{"Node",{{"value",integer()},{"next",NodePtr}}}};
  if(!same(infer(member(variable("n"),"value"),{{"n",Node}}, {}, structs),integer()))throw std::runtime_error("struct field type failed");
  if(!same(infer(arrow(variable("p"),"next"),{{"p",NodePtr}}, {}, structs),NodePtr))throw std::runtime_error("pointer field type failed");
  if(!same(infer(assign(arrow(variable("p"),"value"),literal(7)),{{"p",NodePtr}}, {}, structs),integer()))throw std::runtime_error("assignment type failed");
  if(!same(infer(binary(BinOp::Add,literal(2),literal(3)),{}, {}, structs),integer()))throw std::runtime_error("binary operator type failed");
  auto IntPtr=pointer(integer());
  if(size_of(integer(),structs)!=4||size_of(IntPtr,structs)!=8||size_of(Node,structs)!=12)throw std::runtime_error("layout size failed");
  if(!same(infer(sizeof_type(Node),{}, {}, structs),integer()))throw std::runtime_error("sizeof type failed");
  if(!same(infer(allocate(Node),{}, {}, structs),NodePtr))throw std::runtime_error("allocation type failed");
  if(!same(infer(release(variable("p")),{{"p",NodePtr}}, {}, structs),unit()))throw std::runtime_error("free type failed");
  if(!same(infer(index(variable("buf"),literal(2)),{{"buf",IntPtr}}, {}, structs),integer()))throw std::runtime_error("index type failed");
  if(!same(infer(binary(BinOp::Add,variable("buf"),literal(1)),{{"buf",IntPtr}}, {}, structs),IntPtr))throw std::runtime_error("pointer arithmetic type failed");
  if(!same(infer(conditional(literal(1),literal(2),literal(3)),{}, {}, structs),integer()))throw std::runtime_error("conditional type failed");
  bool bad_assignment=false;try{(void)infer(assign(arrow(variable("p"),"value"),variable("p")),{{"p",NodePtr}}, {}, structs);}catch(std::exception const&){bad_assignment=true;}if(!bad_assignment)throw std::runtime_error("bad assignment accepted");
  bool bad_conditional=false;try{(void)infer(conditional(variable("p"),literal(1),literal(0)),{{"p",NodePtr}}, {}, structs);}catch(std::exception const&){bad_conditional=true;}if(!bad_conditional)throw std::runtime_error("pointer conditional accepted");
  bool bad_index=false;try{(void)infer(index(variable("n"),literal(1)),{{"n",Node}}, {}, structs);}catch(std::exception const&){bad_index=true;}if(!bad_index)throw std::runtime_error("struct value index accepted");
  bool bad_free=false;try{(void)infer(release(variable("n")),{{"n",Node}}, {}, structs);}catch(std::exception const&){bad_free=true;}if(!bad_free)throw std::runtime_error("free of struct accepted");
  Function spin{"spin",{{"p",NodePtr}},unit(),{while_loop(arrow(variable("p"),"value"),assign(arrow(variable("p"),"value"),literal(0)))} };
  Functions loop_fs{{"spin",&spin}}; check(spin,loop_fs,structs);
  bool bad_loop=false;try{(void)infer(while_loop(variable("p"),literal(0)),{{"p",NodePtr}}, {}, structs);}catch(std::exception const&){bad_loop=true;}if(!bad_loop)throw std::runtime_error("pointer while condition accepted");
  Function inc{"inc",{{"x",integer()}},integer(),{binary(BinOp::Add,variable("x"),literal(1))}};
  Functions callback_fs{{"inc",&inc}};
  auto callback_type=function({integer()},integer());
  if(!same(infer(indirect_call(variable("cb"),{literal(4)}),{{"cb",callback_type}},callback_fs,structs),integer()))throw std::runtime_error("indirect call type failed");
  if(!same(infer(function_ref("inc"),{},callback_fs,structs),callback_type))throw std::runtime_error("function reference type failed");
  bool bad_indirect=false;try{(void)infer(indirect_call(variable("cb"),{variable("p")}),{{"cb",callback_type},{"p",NodePtr}},callback_fs,structs);}catch(std::exception const&){bad_indirect=true;}if(!bad_indirect)throw std::runtime_error("bad indirect call accepted");
  Function length{"length",{{"p",NodePtr}},integer(),{conditional(arrow(variable("p"),"value"),call("length",{variable("p")}),literal(0))}};
  Functions fs{{"length",&length}}; check(length,fs,structs); // recursive call is type-checked
  check_body(length,{if_stmt(arrow(variable("p"),"value"),{return_stmt(call("length",{variable("p")}))},{return_stmt(literal(0))})},fs,structs);
  Function mutate{"mutate",{{"p",NodePtr}},unit(),{}};
  Functions body_fs{{"mutate",&mutate}};
  check_body(mutate,{while_stmt(arrow(variable("p"),"value"),{expr_stmt(assign(arrow(variable("p"),"value"),literal(0)))})},body_fs,structs);
  auto bad=Function{"bad",{{"p",NodePtr}},integer(),{dereference(variable("p"))}}; bool rejected=false;try{check(bad,fs,structs);}catch(std::exception const&){rejected=true;}if(!rejected)throw std::runtime_error("bad pointer result accepted");
  using namespace st; auto A=sort(1); auto n=nbe_normalise({},normalize_code(A,quote(app(lam(A,var(0)),sort(0)))));if(!equal(n,quote(sort(0))))throw Error("NbE bridge failed");
  std::cout<<"struct Node: PASS\nstruct fields: PASS\npointer fields: PASS\nlayout and sizeof: PASS\nallocation: PASS\ndeallocation: PASS\nassignments: PASS\nbinary operators: PASS\npointer indexing: PASS\npointer arithmetic: PASS\nconditionals: PASS\nwhile statements: PASS\nstatement bodies: PASS\nfunction references: PASS\nindirect calls: PASS\npointer types: PASS\nfunction calls: PASS\nrecursive call typing: PASS\nNbE bridge: PASS\n";
}catch(std::exception const&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
