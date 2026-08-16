// Integration test for the object-language representation of the normalizer.
// This includes the real NbE/type-checker core; no second evaluator is used.
#define NORMALISER_LIBRARY
#include "normaliser.cpp"

int main() {
  using namespace st;
  try {
    auto A=sort(1);
    auto codeA=code_type(A);
    // Nbody : Code(A) -> Code(A), using the typed object-language primitive
    // normalizeCode rather than a host-side self_apply callback.
    auto Nbody=lam(codeA,normalize_code(A,var(0)));
    auto N=quote(Nbody);
    auto nTy=infer({},N);
    auto expected=code_type(pi(codeA,codeA));
    if(!equal(nTy,expected)) throw Error("quoted normalizer has wrong type: "+show(nTy)+" vs "+show(expected));

    auto input=quote(app(lam(A,var(0)),sort(0)));
    auto self=app(unquote(N),input);
    try { check({},self,codeA); } catch (const Error&) { throw Error("self type: "+show(infer({},self))+" vs "+show(codeA)); }
    auto result=nbe_normalise({},self);
    auto expectedResult=quote(sort(0));
    if(!equal(result,expectedResult)) throw Error("object normalizer did not beta-normalize code: "+show(result)+" vs "+show(expectedResult));

    std::cout << "N is an object-language term: PASS\n"
              << "N : Code(A) -> Code(A): PASS\n"
              << "unquote(N) input: PASS\n"
              << "self-application through NbE: PASS\n";
  } catch (const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
}
