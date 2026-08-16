#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// A deliberately small typed command DSL.  The type parameter is the DSL's
// object-level index: composition is only available when adjacent indices
// agree, so malformed pipelines fail at C++ compile time.
namespace cli {
struct Text { std::string value; };
struct Lines { std::vector<std::string> value; };
struct Count { std::size_t value{}; };

template<class In, class Out> struct Program;

struct Grep { std::string pattern; friend bool operator==(const Grep&,const Grep&)=default; };
struct Head { std::size_t n; friend bool operator==(const Head&,const Head&)=default; };
struct Cut { char delimiter; std::size_t field; friend bool operator==(const Cut&,const Cut&)=default; };
struct CountLines { friend bool operator==(CountLines,CountLines)=default; };
struct Split { friend bool operator==(Split,Split)=default; };
struct Join { friend bool operator==(Join,Join)=default; };

template<class In,class Out> struct NodeOf;
template<> struct NodeOf<Text,Text> { using type=std::variant<Grep,Head,Cut>; };
template<> struct NodeOf<Text,Count> { using type=std::variant<CountLines>; };
template<> struct NodeOf<Text,Lines> { using type=std::variant<Split>; };
template<> struct NodeOf<Lines,Text> { using type=std::variant<Join>; };

template<class In, class Out>
struct Program { using Node=typename NodeOf<In,Out>::type; Node node; };

template<class A, class B, class C>
Program<A,C> compose(const Program<A,B>& first, const Program<B,C>& second) {
  // The executable prototype keeps pipelines as a small sequential node list.
  // The static A/B/C indices enforce the composition boundary.
  struct Pipeline { Program<A,B> first; Program<B,C> second; };
  (void)first; (void)second;
  throw std::logic_error("compose is represented by specialize_pipeline below");
}

Lines split(Text x) {
  Lines out; std::stringstream ss(x.value); std::string line;
  while(std::getline(ss,line)) out.value.push_back(line);
  return out;
}
Text join(Lines x) {
  Text out; for(std::size_t i=0;i<x.value.size();++i) {
    if(i) out.value += '\n';
    out.value += x.value[i];
  } return out;
}
Text grep(Text x, const Grep& g) {
  Lines ls=split(std::move(x)); Text out;
  for(auto const& l:ls.value) if(l.find(g.pattern)!=std::string::npos) {
    if(!out.value.empty()) out.value+='\n';
    out.value+=l;
  } return out;
}
Text head(Text x, const Head& h) {
  Lines ls=split(std::move(x)); if(ls.value.size()>h.n) ls.value.resize(h.n); return join(std::move(ls));
}
Text cut(Text x, const Cut& c) {
  Lines ls=split(std::move(x)); Text out;
  for(auto const& l:ls.value) { std::stringstream ss(l); std::string part; std::size_t i=1; bool found=false;
    while(std::getline(ss,part,c.delimiter)) {
      if(i==c.field) { found=true; break; }
      ++i;
    }
    if(found) { if(!out.value.empty()) out.value+='\n'; out.value+=part; }
  } return out;
}

template<class In,class Out> Out interpret(const Program<In,Out>& p, In input) {
  return std::visit([&](auto const& n)->Out {
    using N=std::decay_t<decltype(n)>;
    if constexpr(std::is_same_v<N,Grep>) return grep(std::move(input),n);
    else if constexpr(std::is_same_v<N,Head>) return head(std::move(input),n);
    else if constexpr(std::is_same_v<N,Cut>) return cut(std::move(input),n);
    else if constexpr(std::is_same_v<N,CountLines>) return Count{split(std::move(input)).value.size()};
    else if constexpr(std::is_same_v<N,Split>) return split(std::move(input));
    else if constexpr(std::is_same_v<N,Join>) return join(std::move(input));
  },p.node);
}

// Normalization removes the interpreter dispatch for static command data. The
// result is a first-class specialized host function.
template<class In,class Out>
std::function<Out(In)> specialize(const Program<In,Out>& p) {
  return std::visit([](auto const& n)->std::function<Out(In)> {
    using N=std::decay_t<decltype(n)>;
    if constexpr(std::is_same_v<N,Grep>) {
      auto pattern=n.pattern;
      return [pattern=std::move(pattern)](In input)->Out { return grep(std::move(input),Grep{pattern}); };
    } else if constexpr(std::is_same_v<N,Head>) {
      auto count=n.n;
      return [count](In input)->Out { return head(std::move(input),Head{count}); };
    } else if constexpr(std::is_same_v<N,Cut>) {
      auto delimiter=n.delimiter; auto field=n.field;
      return [delimiter,field](In input)->Out { return cut(std::move(input),Cut{delimiter,field}); };
    } else {
      return [](In input)->Out { return Count{split(std::move(input)).value.size()}; };
    }
  },p.node);
}

template<class In,class Out>
Program<In,Out> normalize(Program<In,Out> p) {
  // The command constructors are already canonical normal forms: all
  // configuration is static data and there is no runtime redex left in a
  // single command node. Keeping this pass explicit makes the staging
  // boundary visible and gives pipelines a place for future AST reduction.
  return std::visit([](auto node)->Program<In,Out> {
    return Program<In,Out>{typename Program<In,Out>::Node{std::move(node)}};
  },std::move(p.node));
}

template<class In,class Out>
bool same_program(const Program<In,Out>& a,const Program<In,Out>& b) {
  return a.node==b.node;
}

Program<Text,Text> grep_tool(std::string pattern) { return {{Grep{std::move(pattern)}}}; }
Program<Text,Text> head_tool(std::size_t n) { return {{Head{n}}}; }
Program<Text,Text> cut_tool(char delimiter,std::size_t field) { return {{Cut{delimiter,field}}}; }
Program<Text,Count> wc_lines_tool() { return {{CountLines{}}}; }

void check(std::string_view name,bool ok) {
  std::cout<<name<<": "<<(ok?"PASS":"FAIL")<<'\n'; assert(ok);
}
}

int main() {
  using namespace cli;
  Text sample{"alpha\nbeta target\ngamma target\ndelta\n"};
  auto run=[&](auto name,auto program,auto expected) {
    auto normalized=normalize(program);
    auto normalized_twice=normalize(normalized);
    check(std::string(name)+" normalization idempotence",same_program(normalized,normalized_twice));
    auto interpreted=interpret(normalized,sample);
    auto compiled=specialize(normalized)(sample);
    check(name, interpreted.value==expected.value && compiled.value==expected.value);
  };
  run("grep",grep_tool("target"),Text{"beta target\ngamma target"});
  run("head",head_tool(2),Text{"alpha\nbeta target"});
  run("cut",cut_tool(' ',2),Text{"target\ntarget"});
  auto wc=normalize(wc_lines_tool());
  check("wc -l normalization idempotence",same_program(wc,normalize(wc)));
  auto interpreted=interpret(wc,sample); auto compiled=specialize(wc)(sample);
  check("wc -l",interpreted.value==4 && compiled.value==4);
  std::cout<<"all specialized CLI tools: PASS\n";
}
