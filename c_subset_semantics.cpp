// Typed semantic core for the next C-subset layer.  This is deliberately
// separate from the permissive source reader: it gives structs, pointers,
// calls, and recursion real types before assembler lowering.
#define NORMALISER_LIBRARY
#include "normaliser.cpp"
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
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
enum class BinOp { Add, Equal, Subtract, Less, LogicalAnd, LogicalOr };
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
struct Global{std::string name;T type;std::optional<Expr> initializer;};
using Env=std::map<std::string,T>; using Functions=std::map<std::string,Function const*>;
using StructFields=std::map<std::string,std::vector<std::pair<std::string,T>>>;
using Aliases=std::map<std::string,T>;
void add_alias(Aliases&aliases,std::string name,T type){if(!aliases.emplace(std::move(name),std::move(type)).second)throw std::runtime_error("duplicate typedef");}
T resolve_alias(Aliases const&aliases,std::string const&name){std::vector<std::string> path;std::string current=name;for(;;){if(std::find(path.begin(),path.end(),current)!=path.end())throw std::runtime_error("typedef cycle");auto i=aliases.find(current);if(i==aliases.end())throw std::runtime_error("unknown typedef "+current);path.push_back(current);if(auto s=std::get_if<Ty::Struct>(&i->second->n);s&&aliases.find(s->name)!=aliases.end()){current=s->name;continue;}return i->second;}}
T field(T const&base,std::string const&name,StructFields const&ss){auto s=std::get_if<Ty::Struct>(&base->n);if(!s)throw std::runtime_error("field access on non-struct");auto si=ss.find(s->name);if(si==ss.end())throw std::runtime_error("unknown struct "+s->name);for(auto const&f:si->second)if(f.first==name)return f.second;throw std::runtime_error("unknown field "+name);}
size_t size_of(T const&t,StructFields const&ss){return std::visit([&](auto const&q)->size_t{using X=std::decay_t<decltype(q)>;if constexpr(std::is_same_v<X,Ty::Int>)return 4;if constexpr(std::is_same_v<X,Ty::Void>)throw std::runtime_error("sizeof void");if constexpr(std::is_same_v<X,Ty::Ptr>)return 8;if constexpr(std::is_same_v<X,Ty::Fn>)return 8;if constexpr(std::is_same_v<X,Ty::Struct>){auto i=ss.find(q.name);if(i==ss.end())throw std::runtime_error("sizeof unknown struct "+q.name);size_t n=0;for(auto const&f:i->second)n+=size_of(f.second,ss);return n;}},t->n);}
size_t field_offset(T const&t,std::string const&name,StructFields const&ss){auto s=std::get_if<Ty::Struct>(&t->n);if(!s)throw std::runtime_error("offsetof on non-struct");auto i=ss.find(s->name);if(i==ss.end())throw std::runtime_error("offsetof unknown struct "+s->name);size_t off=0;for(auto const&f:i->second){if(f.first==name)return off;off+=size_of(f.second,ss);}throw std::runtime_error("offsetof unknown field "+name);}
bool lvalue(Expr const&x){return std::holds_alternative<Var>(x->n)||std::holds_alternative<Deref>(x->n)||std::holds_alternative<Member>(x->n)||std::holds_alternative<Arrow>(x->n);}
T infer(Expr const&x,Env const&env,Functions const&fs,StructFields const&ss){return std::visit([&](auto const&q)->T{using X=std::decay_t<decltype(q)>;if constexpr(std::is_same_v<X,Lit>)return integer();if constexpr(std::is_same_v<X,Var>){auto i=env.find(q.name);if(i==env.end())throw std::runtime_error("unbound C variable "+q.name);return i->second;}if constexpr(std::is_same_v<X,Addr>){if(!lvalue(q.x))throw std::runtime_error("address of non-lvalue");return pointer(infer(q.x,env,fs,ss));}if constexpr(std::is_same_v<X,Deref>){auto t=infer(q.x,env,fs,ss);auto p=std::get_if<Ty::Ptr>(&t->n);if(!p)throw std::runtime_error("dereference of non-pointer");return p->to;}if constexpr(std::is_same_v<X,Member>)return field(infer(q.base,env,fs,ss),q.field,ss);if constexpr(std::is_same_v<X,Arrow>){auto t=infer(q.base,env,fs,ss);auto p=std::get_if<Ty::Ptr>(&t->n);if(!p)throw std::runtime_error("arrow access on non-pointer");return field(p->to,q.field,ss);}if constexpr(std::is_same_v<X,Binary>){auto l=infer(q.left,env,fs,ss);auto r=infer(q.right,env,fs,ss);if(q.op==BinOp::Add){if(same(l,integer())&&same(r,integer()))return integer();if(auto p=std::get_if<Ty::Ptr>(&l->n);p&&same(r,integer()))return l;if(auto p=std::get_if<Ty::Ptr>(&r->n);p&&same(l,integer()))return r;throw std::runtime_error("pointer addition types");}if(!same(l,r))throw std::runtime_error("equality operand types");return integer();}if constexpr(std::is_same_v<X,Assign>){if(!lvalue(q.left))throw std::runtime_error("assignment to non-lvalue");auto l=infer(q.left,env,fs,ss);auto r=infer(q.right,env,fs,ss);if(!same(l,r))throw std::runtime_error("assignment type mismatch");return l;}if constexpr(std::is_same_v<X,If>){if(!same(infer(q.condition,env,fs,ss),integer()))throw std::runtime_error("conditional requires integer condition");auto y=infer(q.yes,env,fs,ss);auto n=infer(q.no,env,fs,ss);if(!same(y,n))throw std::runtime_error("conditional branch type mismatch");return y;}if constexpr(std::is_same_v<X,While>){if(!same(infer(q.condition,env,fs,ss),integer()))throw std::runtime_error("while requires integer condition");(void)infer(q.body,env,fs,ss);return unit();}if constexpr(std::is_same_v<X,Sequence>){T result=unit();for(auto const&item:q.items)result=infer(item,env,fs,ss);return result;}if constexpr(std::is_same_v<X,Index>){auto b=infer(q.base,env,fs,ss);if(!same(infer(q.index,env,fs,ss),integer()))throw std::runtime_error("index is not integer");auto p=std::get_if<Ty::Ptr>(&b->n);if(!p)throw std::runtime_error("index of non-pointer");return p->to;}if constexpr(std::is_same_v<X,Sizeof>){(void)size_of(q.type,ss);return integer();}if constexpr(std::is_same_v<X,Alloc>){(void)size_of(q.type,ss);return pointer(q.type);}if constexpr(std::is_same_v<X,Free>){auto t=infer(q.value,env,fs,ss);if(!std::holds_alternative<Ty::Ptr>(t->n))throw std::runtime_error("free of non-pointer");return unit();}if constexpr(std::is_same_v<X,FunctionRef>){auto f=fs.find(q.fn);if(f==fs.end())throw std::runtime_error("unknown function reference");std::vector<T> args;for(auto const&a:f->second->args)args.push_back(a.second);return function(std::move(args),f->second->ret);}if constexpr(std::is_same_v<X,Call>){auto f=fs.find(q.fn);if(f==fs.end())throw std::runtime_error("unknown C function "+q.fn);if(f->second->args.size()!=q.args.size())throw std::runtime_error("C call arity");for(size_t i=0;i<q.args.size();++i)if(!same(infer(q.args[i],env,fs,ss),f->second->args[i].second))throw std::runtime_error("C call argument type");return f->second->ret;}if constexpr(std::is_same_v<X,IndirectCall>){auto ft=infer(q.callee,env,fs,ss);auto fn=std::get_if<Ty::Fn>(&ft->n);if(!fn)throw std::runtime_error("indirect call of non-function");if(fn->args.size()!=q.args.size())throw std::runtime_error("indirect call arity");for(size_t i=0;i<q.args.size();++i)if(!same(infer(q.args[i],env,fs,ss),fn->args[i]))throw std::runtime_error("indirect call argument type");return fn->ret;}},x->n);}
void check(Function const&f,Functions const&fs,StructFields const&ss){Env env;for(auto const&a:f.args)env[a.first]=a.second;for(auto const&x:f.body)if(!same(infer(x,env,fs,ss),f.ret))throw std::runtime_error("C function result type");}

struct ReturnStmt{Expr value;}; struct ExprStmt{Expr value;};
struct IfStmt{Expr condition;std::vector<struct Stmt> yes,no;};
struct WhileStmt{Expr condition;std::vector<struct Stmt> body;};
struct ForStmt{Expr init,condition,step;std::vector<struct Stmt> body;};
struct DoWhileStmt{std::vector<struct Stmt> body;Expr condition;};
struct BreakStmt{}; struct ContinueStmt{};
struct Stmt{using N=std::variant<ReturnStmt,ExprStmt,IfStmt,WhileStmt,ForStmt,DoWhileStmt,BreakStmt,ContinueStmt>;N n;template<class X>Stmt(X x):n(std::move(x)){} };
Stmt return_stmt(Expr x){return Stmt(ReturnStmt{std::move(x)});} Stmt expr_stmt(Expr x){return Stmt(ExprStmt{std::move(x)});}
Stmt if_stmt(Expr c,std::vector<Stmt>y,std::vector<Stmt>n){return Stmt(IfStmt{std::move(c),std::move(y),std::move(n)});} Stmt while_stmt(Expr c,std::vector<Stmt>b){return Stmt(WhileStmt{std::move(c),std::move(b)});}
Stmt for_stmt(Expr i,Expr c,Expr step,std::vector<Stmt>b){return Stmt(ForStmt{std::move(i),std::move(c),std::move(step),std::move(b)});}
Stmt do_while_stmt(std::vector<Stmt>b,Expr c){return Stmt(DoWhileStmt{std::move(b),std::move(c)});}
void check_statements(std::vector<Stmt> const&body,T const&ret,Env const&env,Functions const&fs,StructFields const&ss,int loop_depth=0){for(auto const&s:body)std::visit([&](auto const&q){using X=std::decay_t<decltype(q)>;if constexpr(std::is_same_v<X,ReturnStmt>){if(!same(infer(q.value,env,fs,ss),ret))throw std::runtime_error("return type mismatch");}if constexpr(std::is_same_v<X,ExprStmt>){(void)infer(q.value,env,fs,ss);}if constexpr(std::is_same_v<X,IfStmt>){if(!same(infer(q.condition,env,fs,ss),integer()))throw std::runtime_error("if requires integer condition");check_statements(q.yes,ret,env,fs,ss,loop_depth);check_statements(q.no,ret,env,fs,ss,loop_depth);}if constexpr(std::is_same_v<X,WhileStmt>){if(!same(infer(q.condition,env,fs,ss),integer()))throw std::runtime_error("while requires integer condition");check_statements(q.body,unit(),env,fs,ss,loop_depth+1);}if constexpr(std::is_same_v<X,ForStmt>){(void)infer(q.init,env,fs,ss);if(!same(infer(q.condition,env,fs,ss),integer()))throw std::runtime_error("for requires integer condition");(void)infer(q.step,env,fs,ss);check_statements(q.body,unit(),env,fs,ss,loop_depth+1);}if constexpr(std::is_same_v<X,DoWhileStmt>){check_statements(q.body,unit(),env,fs,ss,loop_depth+1);if(!same(infer(q.condition,env,fs,ss),integer()))throw std::runtime_error("do-while requires integer condition");}if constexpr(std::is_same_v<X,BreakStmt>||std::is_same_v<X,ContinueStmt>){if(loop_depth==0)throw std::runtime_error("loop control outside loop");}},s.n);}
bool definitely_returns(std::vector<Stmt> const&body){for(auto const&s:body){bool result=std::visit([](auto const&q)->bool{using X=std::decay_t<decltype(q)>;if constexpr(std::is_same_v<X,ReturnStmt>)return true;if constexpr(std::is_same_v<X,IfStmt>)return definitely_returns(q.yes)&&definitely_returns(q.no);return false;},s.n);if(result)return true;}return false;}
void check_body(Function const&f,std::vector<Stmt> const&body,Functions const&fs,StructFields const&ss){Env env;for(auto const&a:f.args)env[a.first]=a.second;check_statements(body,f.ret,env,fs,ss);if(!same(f.ret,unit())&&!definitely_returns(body))throw std::runtime_error("non-void function may fall through");}
void validate_structs(StructFields const&ss){for(auto const&entry:ss){std::vector<std::string> names;for(auto const&f:entry.second){if(std::find(names.begin(),names.end(),f.first)!=names.end())throw std::runtime_error("duplicate field "+entry.first+"."+f.first);names.push_back(f.first);}}}
void validate_function(Function const&f){std::vector<std::string> names;for(auto const&a:f.args){if(std::find(names.begin(),names.end(),a.first)!=names.end())throw std::runtime_error("duplicate parameter "+f.name+"."+a.first);names.push_back(a.first);}}
void validate_struct_cycles(StructFields const&ss){std::function<bool(std::string const&,std::vector<std::string>&)> visit=[&](std::string const&name,std::vector<std::string>&path){if(std::find(path.begin(),path.end(),name)!=path.end())return true;auto i=ss.find(name);if(i==ss.end())return false;path.push_back(name);for(auto const&f:i->second)if(auto s=std::get_if<Ty::Struct>(&f.second->n);s&&visit(s->name,path)){path.pop_back();return true;}path.pop_back();return false;};for(auto const&entry:ss){std::vector<std::string> path;if(visit(entry.first,path))throw std::runtime_error("by-value recursive struct "+entry.first);}}
void check_program(std::vector<Function> const&program,StructFields const&ss){validate_structs(ss);validate_struct_cycles(ss);Functions fs;for(auto const&f:program){validate_function(f);if(!fs.emplace(f.name,&f).second)throw std::runtime_error("duplicate function "+f.name);}for(auto const&f:program)check(f,fs,ss);}
Env check_globals(std::vector<Global> const&globals,Functions const&fs,StructFields const&ss){Env env;for(auto const&g:globals){if(!env.emplace(g.name,g.type).second)throw std::runtime_error("duplicate global "+g.name);if(g.initializer&&!same(infer(*g.initializer,env,fs,ss),g.type))throw std::runtime_error("global initializer type mismatch");}return env;}
struct SwitchCase{Expr label;std::vector<Stmt> body;};
int constant_label(Expr const&e){auto p=std::get_if<Lit>(&e->n);if(!p)throw std::runtime_error("case label must be constant");return p->value;}
void check_switch(Expr const&selector,std::vector<SwitchCase> const&cases,std::vector<Stmt> const&default_body,T const&ret,Env const&env,Functions const&fs,StructFields const&ss,int loop_depth=0){if(!same(infer(selector,env,fs,ss),integer()))throw std::runtime_error("switch selector must be integer");std::vector<int> labels;for(auto const&c:cases){if(!same(infer(c.label,env,fs,ss),integer()))throw std::runtime_error("case label must be integer");int label=constant_label(c.label);if(std::find(labels.begin(),labels.end(),label)!=labels.end())throw std::runtime_error("duplicate case label");labels.push_back(label);check_statements(c.body,ret,env,fs,ss,loop_depth);}check_statements(default_body,ret,env,fs,ss,loop_depth);}
}

#ifndef CSEM_LIBRARY
int main(){using namespace csem;try{
  auto Node=structure("Node"); auto NodePtr=pointer(Node);
  StructFields structs{{"Node",{{"value",integer()},{"next",NodePtr}}}};
  if(!same(infer(member(variable("n"),"value"),{{"n",Node}}, {}, structs),integer()))throw std::runtime_error("struct field type failed");
  if(!same(infer(arrow(variable("p"),"next"),{{"p",NodePtr}}, {}, structs),NodePtr))throw std::runtime_error("pointer field type failed");
  if(!same(infer(assign(arrow(variable("p"),"value"),literal(7)),{{"p",NodePtr}}, {}, structs),integer()))throw std::runtime_error("assignment type failed");
  if(!same(infer(binary(BinOp::Add,literal(2),literal(3)),{}, {}, structs),integer()))throw std::runtime_error("binary operator type failed");
  if(!same(infer(binary(BinOp::Subtract,literal(3),literal(2)),{}, {}, structs),integer()))throw std::runtime_error("subtraction type failed");
  if(!same(infer(binary(BinOp::Less,literal(2),literal(3)),{}, {}, structs),integer()))throw std::runtime_error("ordering type failed");
  if(!same(infer(binary(BinOp::LogicalAnd,literal(1),literal(0)),{}, {}, structs),integer()))throw std::runtime_error("logical operator type failed");
  auto IntPtr=pointer(integer());
  if(size_of(integer(),structs)!=4||size_of(IntPtr,structs)!=8||size_of(Node,structs)!=12)throw std::runtime_error("layout size failed");
  if(field_offset(Node,"value",structs)!=0||field_offset(Node,"next",structs)!=4)throw std::runtime_error("field offset failed");
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
  auto callback_globals=check_globals({{"cb",callback_type,function_ref("inc")}},callback_fs,structs);
  if(!same(callback_globals.at("cb"),callback_type))throw std::runtime_error("function pointer global type failed");
  Function apply_callback{"apply_callback",{{"cb",callback_type},{"x",integer()}},integer(),{indirect_call(variable("cb"),{variable("x")})}};
  check(apply_callback,callback_fs,structs);
  Function make_callback{"make_callback",{},callback_type,{function_ref("inc")}};
  check(make_callback,callback_fs,structs);
  auto CallbackBox=structure("CallbackBox");
  StructFields callback_structs=structs;
  callback_structs["CallbackBox"]={{"run",callback_type}};
  if(!same(infer(indirect_call(member(variable("box"),"run"),{literal(9)}),{{"box",CallbackBox}},callback_fs,callback_structs),integer()))throw std::runtime_error("struct function pointer field call failed");
  if(!same(infer(indirect_call(arrow(variable("boxp"),"run"),{literal(9)}),{{"boxp",pointer(CallbackBox)}},callback_fs,callback_structs),integer()))throw std::runtime_error("pointer struct function field call failed");
  auto Thunk=structure("Thunk");
  callback_structs["Thunk"]={{"run",callback_type},{"next",pointer(Thunk)}};
  validate_struct_cycles(callback_structs);
  if(size_of(CallbackBox,callback_structs)!=8||field_offset(CallbackBox,"run",callback_structs)!=0)throw std::runtime_error("callback struct layout failed");
  if(size_of(Thunk,callback_structs)!=16||field_offset(Thunk,"next",callback_structs)!=8)throw std::runtime_error("recursive callback layout failed");
  if(!same(infer(indirect_call(arrow(variable("thunk"),"run"),{literal(9)}),{{"thunk",pointer(Thunk)}},callback_fs,callback_structs),integer()))throw std::runtime_error("recursive callback object call failed");
  Functions higher_order_fs{{"inc",&inc},{"make_callback",&make_callback}};
  if(!same(infer(indirect_call(call("make_callback",{}),{literal(9)}),{},higher_order_fs,structs),integer()))throw std::runtime_error("call returned function type failed");
  Function bad_factory{"bad_factory",{},callback_type,{literal(0)}};
  bool bad_factory_rejected=false;try{check(bad_factory,callback_fs,structs);}catch(std::exception const&){bad_factory_rejected=true;}if(!bad_factory_rejected)throw std::runtime_error("bad function-valued return accepted");
  bool bad_indirect=false;try{(void)infer(indirect_call(variable("cb"),{variable("p")}),{{"cb",callback_type},{"p",NodePtr}},callback_fs,structs);}catch(std::exception const&){bad_indirect=true;}if(!bad_indirect)throw std::runtime_error("bad indirect call accepted");
  Function length{"length",{{"p",NodePtr}},integer(),{conditional(arrow(variable("p"),"value"),call("length",{variable("p")}),literal(0))}};
  Functions fs{{"length",&length}}; check(length,fs,structs); // recursive call is type-checked
  std::vector<Function> mutual{{"even",{{"p",NodePtr}},integer(),{call("odd",{variable("p")})}},{"odd",{{"p",NodePtr}},integer(),{call("even",{variable("p")})}}};
  check_program(mutual,structs);
  bool duplicate_function=false;try{check_program({mutual[0],mutual[0]},structs);}catch(std::exception const&){duplicate_function=true;}if(!duplicate_function)throw std::runtime_error("duplicate function accepted");
  bool duplicate_field=false;try{validate_structs({{"Bad",{{"x",integer()},{"x",integer()}}}});}catch(std::exception const&){duplicate_field=true;}if(!duplicate_field)throw std::runtime_error("duplicate field accepted");
  bool duplicate_parameter=false;try{validate_function(Function{"bad_params",{{"p",NodePtr},{"p",NodePtr}},integer(),{literal(0)}});}catch(std::exception const&){duplicate_parameter=true;}if(!duplicate_parameter)throw std::runtime_error("duplicate parameter accepted");
  auto Bad=structure("Bad"); bool value_cycle=false;try{validate_struct_cycles({{"Bad",{{"self",Bad}}}});}catch(std::exception const&){value_cycle=true;}if(!value_cycle)throw std::runtime_error("by-value recursive struct accepted");
  Aliases aliases;add_alias(aliases,"NodeAlias",Node);add_alias(aliases,"NodeAlias2",structure("NodeAlias"));if(!same(resolve_alias(aliases,"NodeAlias2"),Node))throw std::runtime_error("typedef resolution failed");
  bool alias_cycle=false;try{Aliases cyclic;add_alias(cyclic,"A",structure("B"));add_alias(cyclic,"B",structure("A"));(void)resolve_alias(cyclic,"A");}catch(std::exception const&){alias_cycle=true;}if(!alias_cycle)throw std::runtime_error("typedef cycle accepted");
  auto globals=check_globals({{"limit",integer(),literal(4)},{"head",NodePtr,std::nullopt}}, {}, structs);if(globals.size()!=2)throw std::runtime_error("global environment failed");
  bool bad_global=false;try{(void)check_globals({{"bad",integer(),variable("head")}}, {}, structs);}catch(std::exception const&){bad_global=true;}if(!bad_global)throw std::runtime_error("bad global initializer accepted");
  bool duplicate_global=false;try{(void)check_globals({{"x",integer(),std::nullopt},{"x",integer(),std::nullopt}}, {}, structs);}catch(std::exception const&){duplicate_global=true;}if(!duplicate_global)throw std::runtime_error("duplicate global accepted");
  check_body(length,{if_stmt(arrow(variable("p"),"value"),{return_stmt(call("length",{variable("p")}))},{return_stmt(literal(0))})},fs,structs);
  bool missing_return=false;try{check_body(length,{expr_stmt(literal(1))},fs,structs);}catch(std::exception const&){missing_return=true;}if(!missing_return)throw std::runtime_error("fallthrough function accepted");
  Function mutate{"mutate",{{"p",NodePtr}},unit(),{}};
  Functions body_fs{{"mutate",&mutate}};
  check_body(mutate,{while_stmt(arrow(variable("p"),"value"),{expr_stmt(assign(arrow(variable("p"),"value"),literal(0))),Stmt(BreakStmt{}),Stmt(ContinueStmt{})})},body_fs,structs);
  check_body(mutate,{for_stmt(assign(arrow(variable("p"),"value"),literal(0)),arrow(variable("p"),"value"),assign(arrow(variable("p"),"value"),literal(1)),{Stmt(ContinueStmt{})})},body_fs,structs);
  check_body(mutate,{do_while_stmt({expr_stmt(assign(arrow(variable("p"),"value"),literal(0))),Stmt(BreakStmt{})},arrow(variable("p"),"value"))},body_fs,structs);
  bool bad_for=false;try{check_body(mutate,{for_stmt(literal(0),variable("p"),literal(1),{})},body_fs,structs);}catch(std::exception const&){bad_for=true;}if(!bad_for)throw std::runtime_error("pointer for condition accepted");
  bool bad_do=false;try{check_body(mutate,{do_while_stmt({},variable("p"))},body_fs,structs);}catch(std::exception const&){bad_do=true;}if(!bad_do)throw std::runtime_error("pointer do condition accepted");
  Env switch_env{{"p",NodePtr}};
  check_switch(arrow(variable("p"),"value"),{{literal(0),{expr_stmt(assign(arrow(variable("p"),"value"),literal(1)))}}},{expr_stmt(assign(arrow(variable("p"),"value"),literal(2)))},unit(),switch_env,body_fs,structs);
  bool bad_switch=false;try{check_switch(variable("p"),{}, {},unit(),switch_env,body_fs,structs);}catch(std::exception const&){bad_switch=true;}if(!bad_switch)throw std::runtime_error("pointer switch selector accepted");
  bool duplicate_case=false;try{check_switch(literal(1),{{literal(2),{}},{literal(2),{}}},{},unit(),{},body_fs,structs);}catch(std::exception const&){duplicate_case=true;}if(!duplicate_case)throw std::runtime_error("duplicate switch case accepted");
  bool bad_loop_control=false;try{check_body(mutate,{Stmt(BreakStmt{})},body_fs,structs);}catch(std::exception const&){bad_loop_control=true;}if(!bad_loop_control)throw std::runtime_error("top-level break accepted");
  auto bad=Function{"bad",{{"p",NodePtr}},integer(),{dereference(variable("p"))}}; bool rejected=false;try{check(bad,fs,structs);}catch(std::exception const&){rejected=true;}if(!rejected)throw std::runtime_error("bad pointer result accepted");
  using namespace st; auto A=sort(1); auto n=nbe_normalise({},normalize_code(A,quote(app(lam(A,var(0)),sort(0)))));if(!equal(n,quote(sort(0))))throw Error("NbE bridge failed");
  std::cout<<"struct Node: PASS\nstruct fields: PASS\npointer fields: PASS\nlayout and sizeof: PASS\nfield offsets: PASS\nduplicate fields: PASS\nduplicate parameters: PASS\nby-value cycle rejection: PASS\ntypedef aliases: PASS\ntypedef cycle rejection: PASS\nglobal declarations: PASS\nglobal initializer checks: PASS\nduplicate globals: PASS\nallocation: PASS\ndeallocation: PASS\nassignments: PASS\nbinary operators: PASS\nsubtraction and ordering: PASS\nlogical operators: PASS\npointer indexing: PASS\npointer arithmetic: PASS\nconditionals: PASS\nwhile statements: PASS\nfor statements: PASS\ndo-while statements: PASS\nswitch statements: PASS\nduplicate cases: PASS\nloop control: PASS\nstatement bodies: PASS\ndefinite returns: PASS\nmutual recursion: PASS\nduplicate functions: PASS\nfunction references: PASS\nindirect calls: PASS\npointer types: PASS\nfunction calls: PASS\nrecursive call typing: PASS\nNbE bridge: PASS\n";
}catch(std::exception const&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
#endif
