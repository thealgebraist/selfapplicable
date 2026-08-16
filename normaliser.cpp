#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace st {
using Level = std::uint32_t;
using Index = std::uint32_t;
struct Term;
using T = std::shared_ptr<const Term>;
struct Var { Index i; }; struct Sort { Level i; };
struct CodeTy { T type; };
struct Pi { T dom, cod; }; struct Lam { T dom, body; };
struct App { T f, x; }; struct Quote { T body; }; struct Unquote { T code; };
struct NormalizeCode { T type, code; };
struct Term { using Node = std::variant<Var, Sort, CodeTy, Pi, Lam, App, Quote, Unquote, NormalizeCode>; Node n;
  template<class X> explicit Term(X x): n(std::move(x)) {} };
template<class X, class... A> T make(A&&... a) { return std::make_shared<const Term>(X{std::forward<A>(a)...}); }
T var(Index i){return make<Var>(i);} T sort(Level i){return make<Sort>(i);}
T code_type(T a){return make<CodeTy>(a);}
T pi(T a,T b){return make<Pi>(a,b);} T lam(T a,T b){return make<Lam>(a,b);}
T app(T f,T x){return make<App>(f,x);} T quote(T x){return make<Quote>(x);}
T unquote(T x){return make<Unquote>(x);}
T normalize_code(T a,T c){return make<NormalizeCode>(std::move(a),std::move(c));}

T shift(const T& t, int d, Index cutoff=0) {
  return std::visit([&](auto const& x)->T {
    using X=std::decay_t<decltype(x)>;
    if constexpr(std::is_same_v<X,Var>) return var(x.i>=cutoff ? x.i+d : x.i);
    if constexpr(std::is_same_v<X,Sort>) return sort(x.i);
    if constexpr(std::is_same_v<X,CodeTy>) return code_type(shift(x.type,d,cutoff));
    if constexpr(std::is_same_v<X,Pi>) return pi(shift(x.dom,d,cutoff),shift(x.cod,d,cutoff+1));
    if constexpr(std::is_same_v<X,Lam>) return lam(shift(x.dom,d,cutoff),shift(x.body,d,cutoff+1));
    if constexpr(std::is_same_v<X,App>) return app(shift(x.f,d,cutoff),shift(x.x,d,cutoff));
    if constexpr(std::is_same_v<X,Quote>) return quote(shift(x.body,d,cutoff));
    if constexpr(std::is_same_v<X,Unquote>) return unquote(shift(x.code,d,cutoff));
    if constexpr(std::is_same_v<X,NormalizeCode>) return normalize_code(shift(x.type,d,cutoff),shift(x.code,d,cutoff));
  },t->n);
}

struct Error:std::runtime_error { using std::runtime_error::runtime_error; };
using Ctx=std::vector<T>;
T infer(const Ctx&,const T&);

// Substitute the term for variable 0. The result is expressed one binder level out.
T subst0(const T& body, const T& value, Index depth=0) {
  return std::visit([&](auto const& x)->T {
    using X=std::decay_t<decltype(x)>;
    if constexpr(std::is_same_v<X,Var>) {
      if(x.i==depth) return shift(value,static_cast<int>(depth));
      if(x.i>depth) return var(x.i-1);
      return var(x.i);
    }
    if constexpr(std::is_same_v<X,Sort>) return sort(x.i);
    if constexpr(std::is_same_v<X,CodeTy>) return code_type(subst0(x.type,value,depth));
    if constexpr(std::is_same_v<X,Pi>) return pi(subst0(x.dom,value,depth),subst0(x.cod,value,depth+1));
    if constexpr(std::is_same_v<X,Lam>) return lam(subst0(x.dom,value,depth),subst0(x.body,value,depth+1));
    if constexpr(std::is_same_v<X,App>) return app(subst0(x.f,value,depth),subst0(x.x,value,depth));
    if constexpr(std::is_same_v<X,Quote>) return quote(subst0(x.body,value,depth));
    if constexpr(std::is_same_v<X,Unquote>) return unquote(subst0(x.code,value,depth));
    if constexpr(std::is_same_v<X,NormalizeCode>) return normalize_code(subst0(x.type,value,depth),subst0(x.code,value,depth));
  },body->n);
}
T beta(const T& t);
T beta(const T& t) { return std::visit([&](auto const& x)->T {
  using X=std::decay_t<decltype(x)>;
  if constexpr(std::is_same_v<X,Var>||std::is_same_v<X,Sort>) return t;
  if constexpr(std::is_same_v<X,CodeTy>) return code_type(beta(x.type));
  if constexpr(std::is_same_v<X,Pi>) return pi(beta(x.dom),beta(x.cod));
  if constexpr(std::is_same_v<X,Lam>) return lam(beta(x.dom),beta(x.body));
  if constexpr(std::is_same_v<X,Quote>) return quote(x.body); // quotation is inert
  if constexpr(std::is_same_v<X,Unquote>) return beta(x.code); // reduced below by stage evaluator
  if constexpr(std::is_same_v<X,NormalizeCode>) return normalize_code(beta(x.type),beta(x.code));
  if constexpr(std::is_same_v<X,App>) {
    auto f=beta(x.f), a=beta(x.x);
    if(auto l=std::get_if<Lam>(&f->n)) return beta(subst0(l->body,a));
    return app(f,a);
  }
},t->n); }

// NbE semantic domain. Neutral terms are already-normal syntax; closures retain
// the environment required for dependent application.
struct Value;
using V = std::shared_ptr<const Value>;
using Env = std::vector<V>;
struct Neutral { T syntax; };
struct Closure { Env env; T body; };
struct SyntaxValue { T syntax; };
struct Value { using Node=std::variant<Neutral,Closure,SyntaxValue,Sort>; Node n;
  template<class X> explicit Value(X x): n(std::move(x)) {} };
V neutral(T t){return std::make_shared<const Value>(Neutral{std::move(t)});}
T reify(const V&,const T&);
V eval_nbe(const Env&,const T&);
T nbe_normalise(const Ctx&,const T&);
V apply_nbe(const V& f,const V& x) {
  if(auto c=std::get_if<Closure>(&f->n)) { auto e=c->env; e.insert(e.begin(),x); return eval_nbe(e,c->body); }
  if(auto n=std::get_if<Neutral>(&f->n)) return neutral(app(n->syntax,reify(x,sort(0))));
  throw Error("NbE application of non-function");
}
V eval_nbe(const Env& e,const T& t) { return std::visit([&](auto const& x)->V {
  using X=std::decay_t<decltype(x)>;
  if constexpr(std::is_same_v<X,Var>) { if(x.i>=e.size()) throw Error("NbE unbound variable"); return e[x.i]; }
  else if constexpr(std::is_same_v<X,Sort>) return std::make_shared<const Value>(x);
  else if constexpr(std::is_same_v<X,CodeTy>) return std::make_shared<const Value>(Closure{e,x.type});
  else if constexpr(std::is_same_v<X,Lam>) return std::make_shared<const Value>(Closure{e,x.body});
  else if constexpr(std::is_same_v<X,App>) return apply_nbe(eval_nbe(e,x.f),eval_nbe(e,x.x));
  else if constexpr(std::is_same_v<X,Quote>) return std::make_shared<const Value>(SyntaxValue{x.body});
  else if constexpr(std::is_same_v<X,Unquote>) { auto q=eval_nbe(e,x.code); auto s=std::get_if<SyntaxValue>(&q->n); if(!s) throw Error("NbE unquote expects syntax"); return eval_nbe(e,s->syntax); }
  else if constexpr(std::is_same_v<X,NormalizeCode>) { auto q=eval_nbe(e,x.code); auto s=std::get_if<SyntaxValue>(&q->n); if(!s) throw Error("normalizeCode expects syntax"); return std::make_shared<const Value>(SyntaxValue{nbe_normalise({},s->syntax)}); }
  else return std::make_shared<const Value>(Closure{e,x.cod});
},t->n); }
T reify(const V& v,const T& type) {
  if(auto p=std::get_if<Pi>(&type->n)) {
    if(auto n=std::get_if<Neutral>(&v->n))
      return lam(p->dom,app(shift(n->syntax,1),var(0)));
    // The freshly reflected binder is represented at local de Bruijn index 0.
    // Captured neutrals are shifted by the surrounding reification context.
    Index fresh=0; auto z=neutral(var(fresh));
    V result;
    if(auto c=std::get_if<Closure>(&v->n)) { auto e=c->env; e.insert(e.begin(),z); result=eval_nbe(e,c->body); }
    else result=apply_nbe(v,z);
    auto bodyType=subst0(p->cod,var(fresh));
    return lam(p->dom,reify(result,bodyType));
  }
  if(auto n=std::get_if<Neutral>(&v->n)) return n->syntax;
  if(auto s=std::get_if<SyntaxValue>(&v->n)) return quote(s->syntax);
  if(auto s=std::get_if<Sort>(&v->n)) return sort(s->i);
  if(auto c=std::get_if<Closure>(&v->n)) return lam(sort(0),reify(eval_nbe(c->env,c->body),sort(0)));
  throw Error("NbE cannot reify value");
}
Env reflect(const Ctx& c) { Env e; for(Index i=0;i<c.size();++i) e.push_back(neutral(var(i))); return e; }
T nbe_normalise(const Ctx& c,const T& t) { auto a=infer(c,t); return reify(eval_nbe(reflect(c),t),a); }

bool equal(const T&,const T&);
T infer(const Ctx& c,const T& t) { return std::visit([&](auto const& x)->T {
  using X=std::decay_t<decltype(x)>;
  if constexpr(std::is_same_v<X,Var>) { if(x.i>=c.size()) throw Error("unbound variable"); return c[x.i]; }
  if constexpr(std::is_same_v<X,Sort>) return sort(x.i+1);
  if constexpr(std::is_same_v<X,CodeTy>) { infer(c,x.type); return sort(0); }
  if constexpr(std::is_same_v<X,Pi>) {
    auto domainType=infer(c,x.dom);
    auto domainSort=std::get_if<Sort>(&domainType->n);
    if(!domainSort) throw Error("Pi domain is not a type");
    auto d=c; d.insert(d.begin(),x.dom);
    auto codType=infer(d,x.cod);
    auto codSort=std::get_if<Sort>(&codType->n);
    if(!codSort) throw Error("Pi codomain is not a type");
    return sort(std::max(domainSort->i,codSort->i));
  }
  if constexpr(std::is_same_v<X,Lam>) {
    auto domainType=infer(c,x.dom);
    if(!std::holds_alternative<Sort>(domainType->n)) throw Error("lambda domain is not a type");
    auto cc=c; cc.insert(cc.begin(),x.dom); auto bt=infer(cc,x.body); return pi(x.dom,bt);
  }
  if constexpr(std::is_same_v<X,App>) {
    auto ft=infer(c,x.f); auto p=std::get_if<Pi>(&ft->n); if(!p) throw Error("application of non-function");
    check(c,x.x,p->dom); return subst0(p->cod,x.x);
  }
  if constexpr(std::is_same_v<X,Quote>) return code_type(infer(c,x.body));
  if constexpr(std::is_same_v<X,Unquote>) {
    if(auto z=std::get_if<Quote>(&x.code->n)) return infer(c,z->body);
    auto q=infer(c,x.code); auto ct=std::get_if<CodeTy>(&q->n);
    if(!ct) throw Error("unquote expects Code(A)");
    return ct->type;
  }
  if constexpr(std::is_same_v<X,NormalizeCode>) {
    auto qt=infer(c,x.code); auto ct=std::get_if<CodeTy>(&qt->n);
    if(!ct || !equal(ct->type,x.type)) throw Error("normalizeCode expects Code(A)");
    return code_type(x.type);
  }
},t->n); }
void check(const Ctx& c,const T& t,const T& a) {
  if(auto l=std::get_if<Lam>(&t->n)) { auto p=std::get_if<Pi>(&a->n); if(!p||!equal(l->dom,p->dom)) throw Error("lambda domain mismatch"); auto cc=c; cc.insert(cc.begin(),p->dom); check(cc,l->body,p->cod); return; }
  auto got=infer(c,t); if(!equal(got,a)) throw Error("type mismatch");
}
bool equal(const T& a,const T& b) { return std::visit([&](auto const& x,auto const& y)->bool {
  using X=std::decay_t<decltype(x)>; using Y=std::decay_t<decltype(y)>; if constexpr(!std::is_same_v<X,Y>) return false;
  else if constexpr(std::is_same_v<X,Var>) return x.i==y.i; else if constexpr(std::is_same_v<X,Sort>) return x.i==y.i;
  else if constexpr(std::is_same_v<X,CodeTy>) return equal(x.type,y.type);
  else if constexpr(std::is_same_v<X,Pi>) return equal(x.dom,y.dom)&&equal(x.cod,y.cod); else if constexpr(std::is_same_v<X,Lam>) return equal(x.dom,y.dom)&&equal(x.body,y.body);
  else if constexpr(std::is_same_v<X,App>) return equal(x.f,y.f)&&equal(x.x,y.x);
  else if constexpr(std::is_same_v<X,Quote>) return equal(x.body,y.body);
  else if constexpr(std::is_same_v<X,NormalizeCode>) return equal(x.type,y.type)&&equal(x.code,y.code);
  else return equal(x.code,y.code);
},a->n,b->n); }

// Explicit stage boundary: code is inert until unquoted.
struct Code { enum class Kind { Term, Normaliser }; Kind kind; T syntax; };
T nbe_normalise(const Ctx&,const T&);
Code quote_code(T t){return {Code::Kind::Term,std::move(t)};}
Code quote_normaliser(){return {Code::Kind::Normaliser,var(0)};}
T unquote_code(const Code& c,const Ctx& ctx,const T& input){
  if(c.kind==Code::Kind::Normaliser) return nbe_normalise(ctx,input);
  return beta(c.syntax);
}
T normalise(const Ctx& c,const T& t){ auto a=infer(c,t); (void)a; return beta(t); }
Code quoted_normaliser(){ return quote_normaliser(); }
T self_apply(const Ctx& c,const T& t) { auto N=quoted_normaliser(); return unquote_code(N,c,t); }

std::string show(const T& t) { return std::visit([&](auto const& x)->std::string {
  using X=std::decay_t<decltype(x)>;
  if constexpr(std::is_same_v<X,Var>) return "x"+std::to_string(x.i);
  else if constexpr(std::is_same_v<X,Sort>) return "Type"+std::to_string(x.i);
  else if constexpr(std::is_same_v<X,CodeTy>) return "Code("+show(x.type)+")";
  else if constexpr(std::is_same_v<X,Pi>) return "("+show(x.dom)+")->"+show(x.cod);
  else if constexpr(std::is_same_v<X,Lam>) return "(lam "+show(x.body)+")";
  else if constexpr(std::is_same_v<X,App>) return "("+show(x.f)+" "+show(x.x)+")";
  else if constexpr(std::is_same_v<X,Quote>) return "quote("+show(x.body)+")";
  else if constexpr(std::is_same_v<X,NormalizeCode>) return "normalizeCode("+show(x.code)+")";
  else return "unquote("+show(x.code)+")";
},t->n); }
}

#ifndef NORMALISER_LIBRARY
int main() { using namespace st;
  try {
    auto A=sort(0), id=lam(A,lam(shift(A,1),var(0))), a=var(0);
    Ctx ctx{A}; auto term=app(app(id,a),a); check(ctx,term,A); auto n=nbe_normalise(ctx,term);
    if(!equal(nbe_normalise(ctx,n),n)) throw Error("normalization is not idempotent");
    std::cout<<"typed term: "<<show(term)<<"\nnormal form: "<<show(n)<<"\n";
    auto staged=self_apply(ctx,term); if(!equal(n,staged)) throw Error("staged self-application mismatch");
    std::cout<<"staged self-application: "<<show(staged)<<"\nPASS\n";
    auto fnType=pi(A,shift(A,1));
    auto eta=nbe_normalise(Ctx{fnType},var(0));
    std::cout<<"eta-long neutral: "<<show(eta)<<"\n";
    auto normalId=nbe_normalise(ctx,id);
    std::cout<<"eta-long closure: "<<show(normalId)<<"\n";
    auto expectedId=lam(A,lam(shift(A,1),var(0)));
    if(!equal(normalId,expectedId)) throw Error("closure reification failed");
    if(!equal(infer({},sort(0)),sort(1))) throw Error("universe successor failed");
    if(!equal(infer({},fnType),sort(1))) throw Error("Pi universe level failed");
    bool rejected=false;
    try { (void)infer({},app(sort(0),sort(0))); }
    catch(const Error&) { rejected=true; }
    if(!rejected) throw Error("ill-typed application was accepted");
    std::cout<<"universe and rejection tests: PASS\n";
    auto stagedTerm=unquote(quote(a));
    check(ctx,stagedTerm,A);
    if(!equal(nbe_normalise(ctx,stagedTerm),a)) throw Error("typed quote/unquote failed");
    std::cout<<"typed quotation boundary: PASS\n";
    auto substituted=subst0(var(1),var(0));
    if(!equal(substituted,var(0))) throw Error("de Bruijn substitution failed");
    std::cout<<"substitution invariant: PASS\n";

    auto solve=[&](int number,const Ctx& problemCtx,const T& problem) {
      check(problemCtx,problem,infer(problemCtx,problem));
      std::cout<<"problem "<<number<<": "<<show(nbe_normalise(problemCtx,problem))<<"\n";
    };
    // Four elementary problems.
    solve(1,ctx,app(lam(A,var(0)),a));
    solve(2,ctx,app(lam(A,var(0)),app(lam(A,var(0)),a)));
    solve(3,ctx,unquote(quote(app(lam(A,var(0)),a))));
    solve(4,Ctx{fnType},var(0));
    // Four medium problems.
    auto F=pi(A,shift(A,1));
    auto compose=lam(F,
                     lam(shift(F,1),
                         lam(shift(A,2),
                             app(var(2),app(var(1),var(0))))));
    solve(5,ctx,app(app(app(compose,lam(A,var(0))),lam(A,var(0))),a));
    solve(6,ctx,app(app(id,a),a));
    solve(7,ctx,unquote(quote(unquote(quote(app(app(id,a),a))))));
    solve(8,Ctx{fnType,A},app(var(0),var(1)));
  } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
#endif
