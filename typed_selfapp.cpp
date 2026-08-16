// Typed object-language representation of the normalizer.
// CodeAny is the staged syntax universe; Normalizer is an object-language
// constant, not a C++ callback hidden behind self_apply().
#include <iostream>
#include <memory>
#include <stdexcept>
#include <variant>

namespace obj {
struct T; using P=std::shared_ptr<const T>;
struct Type{}; struct CodeAny{}; struct NatType{}; struct BoolType{}; struct StringType{}; struct Var{int i;};
struct Zero{}; struct Succ{P n;};
struct True{}; struct False{};
struct StringNil{}; struct StringCons{char head; P tail;};
struct NatEq{P a,b;}; struct StringEq{P a,b;}; struct BoolIf{P c,t,f;};
struct Pi{P a,b;}; struct Lam{P a,b;}; struct App{P f,x;};
struct Quote{P x;}; struct Unquote{P x;};
// This is the object-language computation rule for normalization.  It is a
// constructor in the syntax universe, not a host callback.
struct NormalizeCase{P input;};
struct T { using N=std::variant<Type,CodeAny,NatType,BoolType,StringType,Var,Zero,Succ,True,False,StringNil,StringCons,NatEq,StringEq,BoolIf,Pi,Lam,App,Quote,Unquote,NormalizeCase>; N n; template<class X>T(X x):n(std::move(x)){} };
template<class X,class... A>P t(A&&...a){return std::make_shared<const T>(X{std::forward<A>(a)...});}
P type(){return t<Type>();} P code(){return t<CodeAny>();}
P nat_type(){return t<NatType>();} P bool_type(){return t<BoolType>();} P string_type(){return t<StringType>();}
P zero(){return t<Zero>();} P succ(P n){return t<Succ>(std::move(n));}
P tru(){return t<True>();} P fls(){return t<False>();}
P strnil(){return t<StringNil>();} P strcons(char h,P tail){return t<StringCons>(h,std::move(tail));}
P nat_eq(P a,P b){return t<NatEq>(std::move(a),std::move(b));}
P string_eq(P a,P b){return t<StringEq>(std::move(a),std::move(b));}
P bool_if(P c,P yes,P no){return t<BoolIf>(std::move(c),std::move(yes),std::move(no));}
P var(int i){return t<Var>(i);} P pi(P a,P b){return t<Pi>(std::move(a),std::move(b));}
P lam(P a,P b){return t<Lam>(std::move(a),std::move(b));} P app(P f,P x){return t<App>(std::move(f),std::move(x));}
P quote(P x){return t<Quote>(std::move(x));} P unquote(P x){return t<Unquote>(std::move(x));}
P normalizer(){return lam(code(),t<NormalizeCase>(var(0)));}

// The typed kernel primitive has the object-language type CodeAny -> CodeAny.
P nty(){return pi(code(),code());}
P infer(P const& x) {
  return std::visit([&](auto const& q)->P {
    using X=std::decay_t<decltype(q)>;
    if constexpr(std::is_same_v<X,Type>) return type();
    if constexpr(std::is_same_v<X,CodeAny>) return type();
    if constexpr(std::is_same_v<X,NatType>||std::is_same_v<X,BoolType>||std::is_same_v<X,StringType>) return type();
    if constexpr(std::is_same_v<X,Zero>||std::is_same_v<X,Succ>) return nat_type();
    if constexpr(std::is_same_v<X,True>||std::is_same_v<X,False>) return bool_type();
    if constexpr(std::is_same_v<X,StringNil>||std::is_same_v<X,StringCons>) return string_type();
    if constexpr(std::is_same_v<X,NatEq>) { if(!std::holds_alternative<NatType>(infer(q.a)->n)||!std::holds_alternative<NatType>(infer(q.b)->n)) throw std::runtime_error("Nat equality"); return bool_type(); }
    if constexpr(std::is_same_v<X,StringEq>) { if(!std::holds_alternative<StringType>(infer(q.a)->n)||!std::holds_alternative<StringType>(infer(q.b)->n)) throw std::runtime_error("String equality"); return bool_type(); }
    if constexpr(std::is_same_v<X,BoolIf>) { if(!std::holds_alternative<BoolType>(infer(q.c)->n)) throw std::runtime_error("Bool case"); return infer(q.t); }
    if constexpr(std::is_same_v<X,Var>) return code(); // closed examples only
    if constexpr(std::is_same_v<X,NormalizeCase>) return code();
    if constexpr(std::is_same_v<X,Pi>) { (void)infer(q.a); (void)infer(q.b); return type(); }
    if constexpr(std::is_same_v<X,Lam>) { (void)infer(q.a); return pi(q.a,infer(q.b)); }
    if constexpr(std::is_same_v<X,Quote>) { (void)infer(q.x); return code(); }
    if constexpr(std::is_same_v<X,Unquote>) {
      if(auto z=std::get_if<Quote>(&q.x->n); z && std::holds_alternative<Lam>(z->x->n)) return nty();
      if(!std::holds_alternative<CodeAny>(infer(q.x)->n)) throw std::runtime_error("unquote");
      return code();
    }
    if constexpr(std::is_same_v<X,App>) { auto f=infer(q.f); if(!std::holds_alternative<Pi>(f->n)) throw std::runtime_error("application"); return std::get<Pi>(f->n).b; }
  },x->n);
}

P normalize(P const& x) {
  return std::visit([&](auto const& q)->P {
    using X=std::decay_t<decltype(q)>;
    if constexpr(std::is_same_v<X,NatEq>) {
      auto a=normalize(q.a), b=normalize(q.b);
      if(std::holds_alternative<Zero>(a->n)&&std::holds_alternative<Zero>(b->n)) return tru();
      if(auto as=std::get_if<Succ>(&a->n)) if(auto bs=std::get_if<Succ>(&b->n)) return normalize(nat_eq(as->n,bs->n));
      return fls();
    }
    if constexpr(std::is_same_v<X,StringEq>) {
      auto a=normalize(q.a), b=normalize(q.b);
      if(std::holds_alternative<StringNil>(a->n)&&std::holds_alternative<StringNil>(b->n)) return tru();
      if(auto as=std::get_if<StringCons>(&a->n)) if(auto bs=std::get_if<StringCons>(&b->n))
        return as->head==bs->head ? normalize(string_eq(as->tail,bs->tail)) : fls();
      return fls();
    }
    if constexpr(std::is_same_v<X,BoolIf>) {
      auto c=normalize(q.c);
      if(std::holds_alternative<True>(c->n)) return normalize(q.t);
      if(std::holds_alternative<False>(c->n)) return normalize(q.f);
      return x;
    }
    if constexpr(std::is_same_v<X,Unquote>) {
      if(auto z=std::get_if<Quote>(&q.x->n)) return quote(normalize(z->x));
      return x;
    }
    if constexpr(std::is_same_v<X,App>) {
      if(auto u=std::get_if<Unquote>(&q.f->n))
        if(auto z=std::get_if<Quote>(&u->x->n); z && std::holds_alternative<Lam>(z->x->n)) return quote(normalize(q.x));
      return x;
    }
    return x;
  },x->n);
}
}

int main(){using namespace obj;
  auto N=quote(normalizer());                 // N is typed object-language code
  auto nTy=infer(normalizer());
  if(!std::holds_alternative<Pi>(nTy->n)) throw std::runtime_error("N is not typed");
  auto integer=zero();
  for(int i=0;i<42;++i) integer=succ(integer); // host builds the ADT term
  auto input=quote(integer);
  auto self=app(unquote(N),input);             // unquote(N) : CodeAny -> CodeAny
  if(!std::holds_alternative<CodeAny>(infer(self)->n)) throw std::runtime_error("self application not typed");
  auto result=normalize(self);
  if(!std::holds_alternative<Quote>(result->n)) throw std::runtime_error("normalizer body did not return code");
  auto equal42=nat_eq(integer,integer);
  if(!std::holds_alternative<True>(normalize(equal42)->n)) throw std::runtime_error("Nat eliminator failed");
  auto selected=bool_if(normalize(equal42),zero(),succ(zero()));
  if(!std::holds_alternative<Zero>(normalize(selected)->n)) throw std::runtime_error("Bool eliminator failed");
  auto hello=strcons('o',strcons('k',strnil()));
  if(!std::holds_alternative<True>(normalize(string_eq(hello,hello))->n)) throw std::runtime_error("String eliminator failed");
  std::cout<<"N : CodeAny -> CodeAny\nself application: typed\nnormal form: CodeAny\nPASS\n";
  std::cout<<"inductive Nat equality and Bool elimination: PASS\n";
  std::cout<<"inductive String equality: PASS\n";
}
