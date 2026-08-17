// Compiler for the minimal total ADT language.
// Input: S-expressions such as (add (nat 2) (if true (nat 3) (nat 4))).
// Output: AArch64 Linux assembly whose _start exits with the normalized value.
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace mini {
struct Nat { std::uint64_t value; };
struct Bool { bool value; };
struct Var { std::size_t index; };
struct Add { struct Term *left; struct Term *right; };
struct Mul { struct Term *left; struct Term *right; };
struct Eq { struct Term *left; struct Term *right; };
struct If { struct Term *condition; struct Term *then_branch; struct Term *else_branch; };
struct Let { struct Term *value; struct Term *body; };
struct Term {
  using Node = std::variant<Nat, Bool, Var, Add, Mul, Eq, If, Let>;
  Node node;
};

// Staged syntax is data.  The compiler deliberately crosses this boundary
// before lowering, so the same normalizer can normalize source programs and
// future quoted compiler passes.
struct Code { Term *syntax; };

struct Parser {
  std::string input;
  std::size_t pos = 0;
  explicit Parser(std::string text) : input(std::move(text)) {}
  void space() { while (pos < input.size() && input[pos] <= ' ') ++pos; }
  bool take(char c) { space(); if (pos < input.size() && input[pos] == c) { ++pos; return true; } return false; }
  void word(const char *expected) {
    space(); const std::size_t start = pos;
    while (pos < input.size() && input[pos] > ' ' && input[pos] != ')') ++pos;
    if (input.substr(start, pos - start) != expected) throw std::runtime_error("expected " + std::string(expected));
  }
  std::uint64_t number() {
    space(); const std::size_t start = pos;
    while (pos < input.size() && input[pos] >= '0' && input[pos] <= '9') ++pos;
    if (start == pos) throw std::runtime_error("expected natural number");
    return std::stoull(input.substr(start, pos - start));
  }
  std::size_t index() { return static_cast<std::size_t>(number()); }
  Term *term() {
    if (take('(')) {
      space(); const std::size_t start = pos;
      while (pos < input.size() && input[pos] > ' ' && input[pos] != ')') ++pos;
      const auto tag = input.substr(start, pos - start);
      Term *result = nullptr;
      if (tag == "nat") result = new Term{Nat{number()}};
      else if (tag == "true") result = new Term{Bool{true}};
      else if (tag == "false") result = new Term{Bool{false}};
      else if (tag == "var") result = new Term{Var{index()}};
      else if (tag == "add") result = new Term{Add{term(), term()}};
      else if (tag == "mul") result = new Term{Mul{term(), term()}};
      else if (tag == "eq") result = new Term{Eq{term(), term()}};
      else if (tag == "if") result = new Term{If{term(), term(), term()}};
      else if (tag == "let") result = new Term{Let{term(), term()}};
      else throw std::runtime_error("unknown constructor: " + tag);
      if (!take(')')) throw std::runtime_error("missing ')' ");
      return result;
    }
    space();
    if (input.compare(pos, 4, "true") == 0) { pos += 4; return new Term{Bool{true}}; }
    if (input.compare(pos, 5, "false") == 0) { pos += 5; return new Term{Bool{false}}; }
    throw std::runtime_error("expected term");
  }
  Term *parse() { Term *t = term(); space(); if (pos != input.size()) throw std::runtime_error("trailing input"); return t; }
};

// This is the executable counterpart of eval0/normalise0 in minimal_total_lang.v.
// The environment is de Bruijn-style: index 0 is the newest let binding.
using Env = std::vector<const Term *>;
Term *normalize_in(const Term *term, const Env &env) {
  if (const auto *n = std::get_if<Nat>(&term->node)) return new Term{*n};
  if (const auto *b = std::get_if<Bool>(&term->node)) return new Term{*b};
  if (const auto *v = std::get_if<Var>(&term->node)) {
    if (v->index >= env.size()) throw std::runtime_error("unbound variable");
    return normalize_in(env[v->index], env);
  }
  if (const auto *a = std::get_if<Add>(&term->node)) {
    auto *left = normalize_in(a->left, env); auto *right = normalize_in(a->right, env);
    const auto l = std::get<Nat>(left->node).value, r = std::get<Nat>(right->node).value;
    return new Term{Nat{l + r}};
  }
  if (const auto *m = std::get_if<Mul>(&term->node)) {
    auto *left = normalize_in(m->left, env); auto *right = normalize_in(m->right, env);
    const auto l = std::get<Nat>(left->node).value, r = std::get<Nat>(right->node).value;
    return new Term{Nat{l * r}};
  }
  if (const auto *e = std::get_if<Eq>(&term->node)) {
    auto *left = normalize_in(e->left, env); auto *right = normalize_in(e->right, env);
    return new Term{Bool{std::get<Nat>(left->node).value == std::get<Nat>(right->node).value}};
  }
  if (const auto *i = std::get_if<If>(&term->node)) {
    auto *condition = normalize_in(i->condition, env);
    return normalize_in(std::get<Bool>(condition->node).value ? i->then_branch : i->else_branch, env);
  }
  const auto &let = std::get<Let>(term->node);
  auto *value = normalize_in(let.value, env);
  Env extended{value};
  extended.insert(extended.end(), env.begin(), env.end());
  return normalize_in(let.body, extended);
}

Term *normalize(const Term *term) {
  return normalize_in(term, {});
}

Code quote(Term *term) { return Code{term}; }
Term *unquote(const Code &code) { return code.syntax; }
Code normalize_code(const Code &code) { return quote(normalize(unquote(code))); }
Code normalize_code_twice(const Code &code) { return normalize_code(normalize_code(code)); }

// Self-application is staged: quote the input, normalize the quoted code, and
// unquote only the resulting code value.  This is the executable analogue of
// stage_normalise0 in minimal_total_lang.v.
Term *self_apply_normalizer(Term *term) {
  const auto once = normalize_code(quote(term));
  const auto twice = normalize_code_twice(once);
  return unquote(twice);
}

std::uint64_t result(const Term *term) {
  if (const auto *n = std::get_if<Nat>(&term->node)) return n->value;
  return std::get<Bool>(term->node).value ? 1 : 0;
}

void emit_mov(std::ostream &out, std::uint64_t value) {
  if (value <= 65535) { out << "    mov x0, #" << value << "\n"; return; }
  out << "    movz x0, #" << (value & 0xffff) << "\n";
  for (unsigned shift = 16; shift < 64; shift += 16) {
    const auto chunk = (value >> shift) & 0xffff;
    if (chunk) out << "    movk x0, #" << chunk << ", lsl #" << shift << "\n";
  }
}
void emit(std::ostream &out, std::uint64_t value) {
  out << ".text\n.global _start\n.type _start, %function\n_start:\n";
  emit_mov(out, value);
  out << "    mov x8, #93\n    svc #0\n.size _start, .-_start\n";
}
}

int main(int argc, char **argv) {
  try {
    std::ostringstream source;
    const bool direct = argc == 3 && std::string(argv[1]) == "--direct";
    const char *path = direct ? argv[2] : (argc == 2 ? argv[1] : nullptr);
    if (path) { std::ifstream file(path); if (!file) throw std::runtime_error("cannot open input"); source << file.rdbuf(); }
    else { std::string line; while (std::getline(std::cin, line)) source << line << '\n'; }
    mini::Parser parser(source.str());
    auto *source_term = parser.parse();
    const auto *normalized = direct ? mini::normalize(source_term) : mini::self_apply_normalizer(source_term);
    mini::emit(std::cout, mini::result(normalized));
    return 0;
  } catch (const std::exception &error) { std::cerr << "minimal-arm64: " << error.what() << "\n"; return 2; }
}
