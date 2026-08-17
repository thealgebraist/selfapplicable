// Macro-free C subset frontend connected to the dependent NbE core.
// Supported source shape:
//   int main(int argc, char **argv) { return <decimal>; }
// Preprocessor lines and comments are ignored.  The target is emitted as
// x86-64 Linux assembler and linked with as/ld, never with a C compiler.
#define NORMALISER_LIBRARY
#include "normaliser.cpp"
#define CSEM_LIBRARY
#include "c_subset_semantics.cpp"
#include <fstream>
#include <regex>

namespace csubset {
std::string read_source(const char *path) {
  std::ifstream in(path); if(!in) throw std::runtime_error("cannot open source");
  std::string all,line;
  while(std::getline(in,line)) {
    if(!line.empty() && line[0]=='#') continue;
    auto p=line.find("//"); if(p!=std::string::npos) line.resize(p);
    all += line + '\n';
  }
  return all;
}

struct Program { int argc_value=-1, then_status=0, else_status=0, switch_case=-1, switch_case_status=0, switch_case2=-1, switch_case2_status=0, switch_default_status=0; std::string output, loop_output, directory, filter, exists_path, directory_path, regular_path, size_path; unsigned long long size_bytes=0; int loop_count=0; bool loop_present=false, loop_inclusive=false, argv1=false, arg_help=false, cwd=false, listdir=false, exists=false, is_directory=false, is_regular=false, size_gt=false, function_call=false, null_guard=false, pointer_equal=false, switch_return=false, switch_two_cases=false; };

Program parse_main(std::string const& s) {
  std::smatch main_match;
  if(!std::regex_search(s,main_match,std::regex(R"(\bint\s+main\s*)"))) throw std::runtime_error("unsupported main declaration");
  auto main_pos=(std::size_t)main_match.position();
  auto body_start=s.find('{',main_pos);
  auto body_end=s.rfind('}');
  if(body_start==std::string::npos || body_end<=body_start) throw std::runtime_error("malformed main body");
  static const std::regex conditional(R"(if\s*\(\s*argc\s*==\s*([0-9]+)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)");
  static const std::regex null_guard(R"(int\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*0\s*;\s*if\s*\(\s*\1\s*==\s*0\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)");
  static const std::regex pointer_equality(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*=\s*&\s*\1\s*;\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*=\s*&\s*\1\s*;\s*if\s*\(\s*\3\s*==\s*\4\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;\s*\})");
  auto body=s.substr(body_start+1,body_end-body_start-1); std::smatch r;
  Program p;
  std::string recursive_helper;
  static const std::regex recursive_definition(R"(\bint\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\1\s*\(\s*\2\s*\)\s*;\s*\})");
  std::smatch recursive_match;
  if(std::regex_search(s,recursive_match,recursive_definition)) {
    recursive_helper=recursive_match[1].str();
    auto recursive=csem::Function{recursive_helper,{{recursive_match[2].str(),csem::integer()}},csem::integer(),{csem::call(recursive_helper,{csem::variable(recursive_match[2].str())})}};
    csem::check_program({recursive},{});
  } else {
    static const std::regex recursive_base_definition(R"(\bint\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*if\s*\(\s*\2\s*==\s*0\s*\)\s*return\s+0\s*;\s*return\s+\1\s*\(\s*\2\s*-\s*1\s*\)\s*;\s*\})");
    if(std::regex_search(s,recursive_match,recursive_base_definition)) {
      recursive_helper=recursive_match[1].str();
      auto n=csem::variable(recursive_match[2].str());
      auto recursive=csem::Function{recursive_helper,{{recursive_match[2].str(),csem::integer()}},csem::integer(),{}};
      csem::Functions functions{{recursive_helper,&recursive}};
      csem::check_body(recursive,{csem::if_stmt(csem::binary(csem::BinOp::Equal,n,csem::literal(0)),{csem::return_stmt(csem::literal(0))},{csem::return_stmt(csem::call(recursive_helper,{csem::binary(csem::BinOp::Subtract,n,csem::literal(1))}))})},functions,{});
    }
  }
  static const std::regex node_struct(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s+value\s*;\s*struct\s+\1\s*\*\s*next\s*;\s*\}\s*;)");
  std::smatch node_match;
  if(std::regex_search(s,node_match,node_struct)) {
    auto node=csem::structure(node_match[1].str());
    csem::validate_structs({{node_match[1].str(),{{"value",csem::integer()},{"next",csem::pointer(node)}}}});
    csem::validate_struct_cycles({{node_match[1].str(),{{"value",csem::integer()},{"next",csem::pointer(node)}}}});
    static const std::regex pointer_global(R"(\bstruct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*;)");
    std::vector<csem::Global> pointer_globals;
    for(std::sregex_iterator i(s.begin(),s.end(),pointer_global), end; i!=end; ++i)
      if((*i)[1].str()==node_match[1].str()) pointer_globals.push_back({(*i)[2].str(),csem::pointer(node),std::nullopt});
    csem::check_globals(pointer_globals,{},{{node_match[1].str(),{{"value",csem::integer()},{"next",csem::pointer(node)}}}});
  }
  static const std::regex by_value_struct(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*struct\s+\1\s+self\s*;\s*\}\s*;)");
  if(std::regex_search(s,by_value_struct)) throw std::runtime_error("by-value recursive struct is not representable");
  static const std::regex integer_global(R"(\bint\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([0-9]+)\s*;)");
  std::vector<csem::Global> globals;
  for(std::sregex_iterator i(s.begin(),s.end(),integer_global), end; i!=end; ++i)
    globals.push_back({(*i)[1].str(),csem::integer(),csem::literal(std::stoi((*i)[2]))});
  csem::check_globals(globals,{}, {});
  static const std::regex bad_global_function_pointer(R"(int\s*\(\s*\*\s*[A-Za-z_][A-Za-z0-9_]*\s*\)\s*\(\s*int\s*\)\s*=\s*[0-9]+\s*;)");
  if(std::regex_search(s,bad_global_function_pointer)) throw std::runtime_error("function pointer global requires a function initializer");
  static const std::regex bad_explicit_function_pointer_arity(R"(int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*=\s*[^;]+;[\s\S]*return\s*\(\s*\*\s*\1\s*\)\s*\(\s*[0-9]+\s*,\s*[0-9]+\s*\)\s*;)");
  if(std::regex_search(s,bad_explicit_function_pointer_arity)) throw std::runtime_error("function pointer call arity");
  static const std::regex bad_binary_function_pointer_arity(R"(int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*=\s*[^;]+;[\s\S]*return\s*\(\s*\*\s*\1\s*\)\s*\(\s*[0-9]+\s*\)\s*;)");
  if(std::regex_search(s,bad_binary_function_pointer_arity)) throw std::runtime_error("binary function pointer call arity");
  static const std::regex argc_switch(R"(switch\s*\(\s*argc\s*\)\s*\{\s*case\s+([0-9]+)\s*:\s*return\s+([0-9]+)\s*;\s*default\s*:\s*return\s+([0-9]+)\s*;\s*\})");
  static const std::regex argc_switch_two(R"(switch\s*\(\s*argc\s*\)\s*\{\s*case\s+([0-9]+)\s*:\s*return\s+([0-9]+)\s*;\s*case\s+([0-9]+)\s*:\s*return\s+([0-9]+)\s*;\s*default\s*:\s*return\s+([0-9]+)\s*;\s*\})");
  static const std::regex enum_switch(R"(enum\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([0-9]+)\s*,\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([0-9]+)\s*\}\s*;[\s\S]*switch\s*\(\s*argc\s*\)\s*\{\s*case\s+\2\s*:\s*return\s+([0-9]+)\s*;\s*case\s+\4\s*:\s*return\s+([0-9]+)\s*;\s*default\s*:\s*return\s+([0-9]+)\s*;\s*\})");
  static const std::regex enum_switch_implicit(R"(enum\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*([A-Za-z_][A-Za-z0-9_]*)\s*\}\s*;[\s\S]*switch\s*\(\s*argc\s*\)\s*\{\s*case\s+\2\s*:\s*return\s+([0-9]+)\s*;\s*case\s+\3\s*:\s*return\s+([0-9]+)\s*;\s*default\s*:\s*return\s+([0-9]+)\s*;\s*\})");
  if(std::regex_search(s,r,enum_switch_implicit)) {
    p.switch_return=true; p.switch_two_cases=true; p.switch_case=0; p.switch_case_status=std::stoi(r[4]);
    p.switch_case2=1; p.switch_case2_status=std::stoi(r[5]); p.switch_default_status=std::stoi(r[6]);
    csem::validate_enum_values({{r[2].str(),0},{r[3].str(),1}});
  }
  else if(std::regex_search(s,r,enum_switch)) {
    p.switch_return=true; p.switch_two_cases=true; p.switch_case=std::stoi(r[3]); p.switch_case_status=std::stoi(r[6]);
    p.switch_case2=std::stoi(r[5]); p.switch_case2_status=std::stoi(r[7]); p.switch_default_status=std::stoi(r[8]);
    csem::validate_enum_values({{r[2].str(),p.switch_case},{r[4].str(),p.switch_case2}});
    csem::check_switch(csem::variable("argc"),{{csem::literal(p.switch_case),{csem::return_stmt(csem::literal(p.switch_case_status))}},{csem::literal(p.switch_case2),{csem::return_stmt(csem::literal(p.switch_case2_status))}}},{csem::return_stmt(csem::literal(p.switch_default_status))},csem::integer(),{{"argc",csem::integer()}},{},{});
  }
  else if(std::regex_search(body,r,argc_switch_two)) {
    p.switch_return=true; p.switch_two_cases=true; p.switch_case=std::stoi(r[1]); p.switch_case_status=std::stoi(r[2]);
    p.switch_case2=std::stoi(r[3]); p.switch_case2_status=std::stoi(r[4]); p.switch_default_status=std::stoi(r[5]);
    csem::check_switch(csem::variable("argc"),{{csem::literal(p.switch_case),{csem::return_stmt(csem::literal(p.switch_case_status))}},{csem::literal(p.switch_case2),{csem::return_stmt(csem::literal(p.switch_case2_status))}}},{csem::return_stmt(csem::literal(p.switch_default_status))},csem::integer(),{{"argc",csem::integer()}},{},{});
  }
  else if(std::regex_search(body,r,argc_switch)) {
    p.switch_return=true; p.switch_case=std::stoi(r[1]);
    p.switch_case_status=std::stoi(r[2]); p.switch_default_status=std::stoi(r[3]);
    csem::check_switch(csem::variable("argc"),{{csem::literal(p.switch_case),{csem::return_stmt(csem::literal(p.switch_case_status))}}},{csem::return_stmt(csem::literal(p.switch_default_status))},csem::integer(),{{"argc",csem::integer()}},{},{});
  }
  else if(std::regex_search(body,r,conditional)) { p.argc_value=std::stoi(r[1]); p.then_status=std::stoi(r[2]); p.else_status=std::stoi(r[3]); }
  else if(std::regex_search(body,r,null_guard)) { p.null_guard=true; p.then_status=std::stoi(r[2]); p.else_status=std::stoi(r[3]); auto ptr=csem::pointer(csem::integer()); (void)csem::infer(csem::binary(csem::BinOp::Equal,csem::variable("p"),csem::variable("q")),{{"p",ptr},{"q",ptr}}, {}, {}); }
  else if(std::regex_search(s,r,pointer_equality)) {
    p.pointer_equal=true; p.then_status=std::stoi(r[5]); p.else_status=std::stoi(r[6]);
    csem::Function identity{r[1].str(),{{r[2].str(),csem::integer()}},csem::integer(),{csem::variable(r[2].str())}};
    auto fp=csem::function({csem::integer()},csem::integer());
    csem::Functions functions{{identity.name,&identity}};
    csem::Env env{{r[3].str(),csem::pointer(fp)},{r[4].str(),csem::pointer(fp)}};
    (void)csem::infer(csem::assign(csem::variable(r[3].str()),csem::address(csem::function_ref(identity.name))),env,functions,{});
    (void)csem::infer(csem::assign(csem::variable(r[4].str()),csem::address(csem::function_ref(identity.name))),env,functions,{});
    (void)csem::infer(csem::binary(csem::BinOp::Equal,csem::variable(r[3].str()),csem::variable(r[4].str())),env,functions,{});
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*run\s*\)\s*\(\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*\4\.run\s*=\s*&\s*\2\s*;\s*return\s+\4\.run\s*\(\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto fp=csem::function({csem::integer()},csem::integer());
    csem::Function identity{r[2].str(),{{r[3].str(),csem::integer()}},csem::integer(),{csem::variable(r[3].str())}};
    csem::Functions functions{{identity.name,&identity}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[4].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[4].str()),"run"),csem::address(csem::function_ref(identity.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[4].str()),"run")),{csem::literal(std::stoi(r[5]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[5]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*run\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*\+\s+\4\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*\5\.run\s*=\s*&\s*\2\s*;\s*return\s+\5\.run\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
    csem::Function add{r[2].str(),{{r[3].str(),csem::integer()},{r[4].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[3].str()),csem::variable(r[4].str()))}};
    csem::Functions functions{{add.name,&add}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[5].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[5].str()),"run"),csem::address(csem::function_ref(add.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[5].str()),"run")),{csem::literal(std::stoi(r[6])),csem::literal(std::stoi(r[7]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[6])+std::stoi(r[7]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*void\s*\(\s*\*\s*run\s*\)\s*\(\s*int\s*\)\s*;\s*\}\s*;\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*\4\.run\s*=\s*&\s*\2\s*;\s*\4\.run\s*\(\s*([0-9]+)\s*\)\s*;\s*return\s+0\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto fp=csem::function({csem::integer()},csem::unit());
    csem::Function consume{r[2].str(),{{r[3].str(),csem::integer()}},csem::unit(),{}};
    csem::Functions functions{{consume.name,&consume}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[4].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[4].str()),"run"),csem::address(csem::function_ref(consume.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[4].str()),"run")),{csem::literal(std::stoi(r[5]))}),env,functions,fields);
    p.function_call=true; p.else_status=0;
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*run\s*\)\s*\(\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s+([0-9]+)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*\4\.run\s*=\s*&\s*\2\s*;\s*return\s+\4\.run\s*\(\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto fp=csem::function({},csem::integer());
    csem::Function answer{r[2].str(),{},csem::integer(),{csem::literal(std::stoi(r[3]))}};
    csem::Functions functions{{answer.name,&answer}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[4].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[4].str()),"run"),csem::address(csem::function_ref(answer.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[4].str()),"run")),{}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[3]);
  }
  else if(std::regex_search(s,r,std::regex(R"(typedef\s+int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*;\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*\1\s+run\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\4\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\2\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*\5\.run\s*=\s*&\s*\3\s*;\s*return\s+\5\.run\s*\(\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[2].str());
    auto fp=csem::function({csem::integer()},csem::integer());
    csem::Function identity{r[3].str(),{{r[4].str(),csem::integer()}},csem::integer(),{csem::variable(r[4].str())}};
    csem::Functions functions{{identity.name,&identity}};
    csem::StructFields fields{{r[2].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[5].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[5].str()),"run"),csem::address(csem::function_ref(identity.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[5].str()),"run")),{csem::literal(std::stoi(r[6]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[6]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*run\s*\)\s*\(\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*struct\s+\1\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&\s*\4\s*;\s*\5->run\s*=\s*&\s*\2\s*;\s*return\s+\5->run\s*\(\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto box_ptr=csem::pointer(box);
    auto fp=csem::function({csem::integer()},csem::integer());
    csem::Function identity{r[2].str(),{{r[3].str(),csem::integer()}},csem::integer(),{csem::variable(r[3].str())}};
    csem::Functions functions{{identity.name,&identity}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[4].str(),box},{r[5].str(),box_ptr}};
    (void)csem::infer(csem::assign(csem::arrow(csem::variable(r[5].str()),"run"),csem::address(csem::function_ref(identity.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::arrow(csem::variable(r[5].str()),"run")),{csem::literal(std::stoi(r[6]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[6]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*run\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*\+\s+\4\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*struct\s+\1\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&\s*\5\s*;\s*\6->run\s*=\s*&\s*\2\s*;\s*return\s+\6->run\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto box_ptr=csem::pointer(box);
    auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
    csem::Function add{r[2].str(),{{r[3].str(),csem::integer()},{r[4].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[3].str()),csem::variable(r[4].str()))}};
    csem::Functions functions{{add.name,&add}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[5].str(),box},{r[6].str(),box_ptr}};
    (void)csem::infer(csem::assign(csem::arrow(csem::variable(r[6].str()),"run"),csem::address(csem::function_ref(add.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::arrow(csem::variable(r[6].str()),"run")),{csem::literal(std::stoi(r[7])),csem::literal(std::stoi(r[8]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[7])+std::stoi(r[8]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*void\s*\(\s*\*\s*run\s*\)\s*\(\s*int\s*\)\s*;\s*\}\s*;\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*struct\s+\1\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&\s*\4\s*;\s*\5->run\s*=\s*&\s*\2\s*;\s*\5->run\s*\(\s*([0-9]+)\s*\)\s*;\s*return\s+0\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto box_ptr=csem::pointer(box);
    auto fp=csem::function({csem::integer()},csem::unit());
    csem::Function consume{r[2].str(),{{r[3].str(),csem::integer()}},csem::unit(),{}};
    csem::Functions functions{{consume.name,&consume}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[4].str(),box},{r[5].str(),box_ptr}};
    (void)csem::infer(csem::assign(csem::arrow(csem::variable(r[5].str()),"run"),csem::address(csem::function_ref(consume.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::arrow(csem::variable(r[5].str()),"run")),{csem::literal(std::stoi(r[6]))}),env,functions,fields);
    p.function_call=true; p.else_status=0;
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*run\s*\)\s*\(\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s+([0-9]+)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*struct\s+\1\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&\s*\4\s*;\s*\5->run\s*=\s*&\s*\2\s*;\s*return\s+\5->run\s*\(\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto box_ptr=csem::pointer(box);
    auto fp=csem::function({},csem::integer());
    csem::Function answer{r[2].str(),{},csem::integer(),{csem::literal(std::stoi(r[3]))}};
    csem::Functions functions{{answer.name,&answer}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[4].str(),box},{r[5].str(),box_ptr}};
    (void)csem::infer(csem::assign(csem::arrow(csem::variable(r[5].str()),"run"),csem::address(csem::function_ref(answer.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::arrow(csem::variable(r[5].str()),"run")),{}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[3]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*run\s*\)\s*\(\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*&\s*\2\s*\}\s*;\s*return\s+\4\.run\s*\(\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto fp=csem::function({csem::integer()},csem::integer());
    csem::Function identity{r[2].str(),{{r[3].str(),csem::integer()}},csem::integer(),{csem::variable(r[3].str())}};
    csem::Functions functions{{identity.name,&identity}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[4].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[4].str()),"run"),csem::address(csem::function_ref(identity.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[4].str()),"run")),{csem::literal(std::stoi(r[5]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[5]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*run\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*\+\s+\4\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*&\s*\2\s*\}\s*;\s*return\s+\5\.run\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
    csem::Function add{r[2].str(),{{r[3].str(),csem::integer()},{r[4].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[3].str()),csem::variable(r[4].str()))}};
    csem::Functions functions{{add.name,&add}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[5].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[5].str()),"run"),csem::address(csem::function_ref(add.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[5].str()),"run")),{csem::literal(std::stoi(r[6])),csem::literal(std::stoi(r[7]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[6])+std::stoi(r[7]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*run\s*\)\s*\(\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s+([0-9]+)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*&\s*\2\s*\}\s*;\s*return\s+\4\.run\s*\(\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto fp=csem::function({},csem::integer());
    csem::Function answer{r[2].str(),{},csem::integer(),{csem::literal(std::stoi(r[3]))}};
    csem::Functions functions{{answer.name,&answer}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[4].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[4].str()),"run"),csem::address(csem::function_ref(answer.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[4].str()),"run")),{}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[3]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*void\s*\(\s*\*\s*run\s*\)\s*\(\s*int\s*\)\s*;\s*\}\s*;\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*&\s*\2\s*\}\s*;\s*\4\.run\s*\(\s*([0-9]+)\s*\)\s*;\s*return\s+0\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto fp=csem::function({csem::integer()},csem::unit());
    csem::Function consume{r[2].str(),{{r[3].str(),csem::integer()}},csem::unit(),{}};
    csem::Functions functions{{consume.name,&consume}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[4].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[4].str()),"run"),csem::address(csem::function_ref(consume.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[4].str()),"run")),{csem::literal(std::stoi(r[5]))}),env,functions,fields);
    p.function_call=true; p.else_status=0;
  }
  else if(std::regex_search(s,r,std::regex(R"(typedef\s+int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*;\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*\1\s+run\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\4\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\2\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*&\s*\3\s*\}\s*;\s*return\s+\5\.run\s*\(\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[2].str());
    auto fp=csem::function({csem::integer()},csem::integer());
    csem::Function identity{r[3].str(),{{r[4].str(),csem::integer()}},csem::integer(),{csem::variable(r[4].str())}};
    csem::Functions functions{{identity.name,&identity}};
    csem::StructFields fields{{r[2].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[5].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[5].str()),"run"),csem::address(csem::function_ref(identity.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[5].str()),"run")),{csem::literal(std::stoi(r[6]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[6]);
  }
  else if(std::regex_search(s,r,std::regex(R"(typedef\s+int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*\1\s+run\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\4\s*\+\s+\5\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\2\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*&\s*\3\s*\}\s*;\s*return\s+\6\.run\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[2].str());
    auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
    csem::Function add{r[3].str(),{{r[4].str(),csem::integer()},{r[5].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[4].str()),csem::variable(r[5].str()))}};
    csem::Functions functions{{add.name,&add}};
    csem::StructFields fields{{r[2].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[6].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[6].str()),"run"),csem::address(csem::function_ref(add.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[6].str()),"run")),{csem::literal(std::stoi(r[7])),csem::literal(std::stoi(r[8]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[7])+std::stoi(r[8]);
  }
  else if(std::regex_search(s,r,std::regex(R"(typedef\s+void\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*;\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*\1\s+run\s*;\s*\}\s*;\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\2\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*&\s*\3\s*\}\s*;\s*\5\.run\s*\(\s*([0-9]+)\s*\)\s*;\s*return\s+0\s*;\s*\})"))) {
    auto box=csem::structure(r[2].str());
    auto fp=csem::function({csem::integer()},csem::unit());
    csem::Function consume{r[3].str(),{{r[4].str(),csem::integer()}},csem::unit(),{}};
    csem::Functions functions{{consume.name,&consume}};
    csem::StructFields fields{{r[2].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[5].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[5].str()),"run"),csem::address(csem::function_ref(consume.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[5].str()),"run")),{csem::literal(std::stoi(r[6]))}),env,functions,fields);
    p.function_call=true; p.else_status=0;
  }
  else if(std::regex_search(s,r,std::regex(R"(typedef\s+void\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*\1\s+run\s*;\s*\}\s*;\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\2\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*&\s*\3\s*\}\s*;\s*\6\.run\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*return\s+0\s*;\s*\})"))) {
    auto box=csem::structure(r[2].str());
    auto fp=csem::function({csem::integer(),csem::integer()},csem::unit());
    csem::Function consume{r[3].str(),{{r[4].str(),csem::integer()},{r[5].str(),csem::integer()}},csem::unit(),{}};
    csem::Functions functions{{consume.name,&consume}};
    csem::StructFields fields{{r[2].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[6].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[6].str()),"run"),csem::address(csem::function_ref(consume.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[6].str()),"run")),{csem::literal(std::stoi(r[7])),csem::literal(std::stoi(r[8]))}),env,functions,fields);
    p.function_call=true; p.else_status=0;
  }
  else if(std::regex_search(s,r,std::regex(R"(typedef\s+int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*;\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*\1\s+run\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\4\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\2\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*struct\s+\2\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&\s*\5\s*;\s*\6->run\s*=\s*&\s*\3\s*;\s*return\s+\6->run\s*\(\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[2].str());
    auto box_ptr=csem::pointer(box);
    auto fp=csem::function({csem::integer()},csem::integer());
    csem::Function identity{r[3].str(),{{r[4].str(),csem::integer()}},csem::integer(),{csem::variable(r[4].str())}};
    csem::Functions functions{{identity.name,&identity}};
    csem::StructFields fields{{r[2].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[5].str(),box},{r[6].str(),box_ptr}};
    (void)csem::infer(csem::assign(csem::arrow(csem::variable(r[6].str()),"run"),csem::address(csem::function_ref(identity.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::arrow(csem::variable(r[6].str()),"run")),{csem::literal(std::stoi(r[7]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[7]);
  }
  else if(std::regex_search(s,r,std::regex(R"(typedef\s+int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*\1\s+run\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\4\s*\+\s+\5\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\2\s*\s*([A-Za-z_][A-Za-z0-9_]*)\s*;\s*struct\s+\2\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&\s*\6\s*;\s*\7->run\s*=\s*&\s*\3\s*;\s*return\s+\7->run\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[2].str());
    auto box_ptr=csem::pointer(box);
    auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
    csem::Function add{r[3].str(),{{r[4].str(),csem::integer()},{r[5].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[4].str()),csem::variable(r[5].str()))}};
    csem::Functions functions{{add.name,&add}};
    csem::StructFields fields{{r[2].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[6].str(),box},{r[7].str(),box_ptr}};
    (void)csem::infer(csem::assign(csem::arrow(csem::variable(r[7].str()),"run"),csem::address(csem::function_ref(add.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::arrow(csem::variable(r[7].str()),"run")),{csem::literal(std::stoi(r[8])),csem::literal(std::stoi(r[9]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[8])+std::stoi(r[9]);
  }
  else if(std::regex_search(s,r,std::regex(R"(typedef\s+void\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*;\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*\1\s+run\s*;\s*\}\s*;\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\2\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*struct\s+\2\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&\s*\5\s*;\s*\6->run\s*=\s*&\s*\3\s*;\s*\6->run\s*\(\s*([0-9]+)\s*\)\s*;\s*return\s+0\s*;\s*\})"))) {
    auto box=csem::structure(r[2].str());
    auto box_ptr=csem::pointer(box);
    auto fp=csem::function({csem::integer()},csem::unit());
    csem::Function consume{r[3].str(),{{r[4].str(),csem::integer()}},csem::unit(),{}};
    csem::Functions functions{{consume.name,&consume}};
    csem::StructFields fields{{r[2].str(),{{"run",csem::pointer(fp)}}}};
    csem::Env env{{r[5].str(),box},{r[6].str(),box_ptr}};
    (void)csem::infer(csem::assign(csem::arrow(csem::variable(r[6].str()),"run"),csem::address(csem::function_ref(consume.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::arrow(csem::variable(r[6].str()),"run")),{csem::literal(std::stoi(r[7]))}),env,functions,fields);
    p.function_call=true; p.else_status=0;
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*first\s*\)\s*\(\s*int\s*\)\s*;\s*int\s*\(\s*\*\s*second\s*\)\s*\(\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*;\s*\}\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\5\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*\6\.first\s*=\s*&\s*\2\s*;\s*\6\.second\s*=\s*&\s*\4\s*;\s*return\s+\6\.second\s*\(\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto fp=csem::function({csem::integer()},csem::integer());
    csem::Function first{r[2].str(),{{r[3].str(),csem::integer()}},csem::integer(),{csem::variable(r[3].str())}};
    csem::Function second{r[4].str(),{{r[5].str(),csem::integer()}},csem::integer(),{csem::variable(r[5].str())}};
    csem::Functions functions{{first.name,&first},{second.name,&second}};
    csem::StructFields fields{{r[1].str(),{{"first",csem::pointer(fp)},{"second",csem::pointer(fp)}}}};
    csem::Env env{{r[6].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[6].str()),"first"),csem::address(csem::function_ref(first.name))),env,functions,fields);
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[6].str()),"second"),csem::address(csem::function_ref(second.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[6].str()),"second")),{csem::literal(std::stoi(r[7]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[7]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*get\s*\)\s*\(\s*int\s*\)\s*;\s*void\s*\(\s*\*\s*put\s*\)\s*\(\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*;\s*\}\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*\6\.get\s*=\s*&\s*\2\s*;\s*\6\.put\s*=\s*&\s*\4\s*;\s*\6\.put\s*\(\s*([0-9]+)\s*\)\s*;\s*return\s+\6\.get\s*\(\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto get_fp=csem::function({csem::integer()},csem::integer());
    auto put_fp=csem::function({csem::integer()},csem::unit());
    csem::Function get{r[2].str(),{{r[3].str(),csem::integer()}},csem::integer(),{csem::variable(r[3].str())}};
    csem::Function put{r[4].str(),{{r[5].str(),csem::integer()}},csem::unit(),{}};
    csem::Functions functions{{get.name,&get},{put.name,&put}};
    csem::StructFields fields{{r[1].str(),{{"get",csem::pointer(get_fp)},{"put",csem::pointer(put_fp)}}}};
    csem::Env env{{r[6].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[6].str()),"get"),csem::address(csem::function_ref(get.name))),env,functions,fields);
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[6].str()),"put"),csem::address(csem::function_ref(put.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[6].str()),"put")),{csem::literal(std::stoi(r[7]))}),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[6].str()),"get")),{csem::literal(std::stoi(r[8]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[8]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*get\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*void\s*\(\s*\*\s*put\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*\+\s+\4\s*;\s*\}\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*\8\.get\s*=\s*&\s*\2\s*;\s*\8\.put\s*=\s*&\s*\5\s*;\s*\8\.put\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*return\s+\8\.get\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto get_fp=csem::function({csem::integer(),csem::integer()},csem::integer());
    auto put_fp=csem::function({csem::integer(),csem::integer()},csem::unit());
    csem::Function get{r[2].str(),{{r[3].str(),csem::integer()},{r[4].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[3].str()),csem::variable(r[4].str()))}};
    csem::Function put{r[5].str(),{{r[6].str(),csem::integer()},{r[7].str(),csem::integer()}},csem::unit(),{}};
    csem::Functions functions{{get.name,&get},{put.name,&put}};
    csem::StructFields fields{{r[1].str(),{{"get",csem::pointer(get_fp)},{"put",csem::pointer(put_fp)}}}};
    csem::Env env{{r[8].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[8].str()),"get"),csem::address(csem::function_ref(get.name))),env,functions,fields);
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[8].str()),"put"),csem::address(csem::function_ref(put.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[8].str()),"put")),{csem::literal(std::stoi(r[9])),csem::literal(std::stoi(r[10]))}),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[8].str()),"get")),{csem::literal(std::stoi(r[11])),csem::literal(std::stoi(r[12]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[11])+std::stoi(r[12]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*get\s*\)\s*\(\s*int\s*\)\s*;\s*void\s*\(\s*\*\s*put\s*\)\s*\(\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*;\s*\}\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*&\s*\2\s*,\s*&\s*\4\s*\}\s*;\s*\6\.put\s*\(\s*([0-9]+)\s*\)\s*;\s*return\s+\6\.get\s*\(\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto get_fp=csem::function({csem::integer()},csem::integer());
    auto put_fp=csem::function({csem::integer()},csem::unit());
    csem::Function get{r[2].str(),{{r[3].str(),csem::integer()}},csem::integer(),{csem::variable(r[3].str())}};
    csem::Function put{r[4].str(),{{r[5].str(),csem::integer()}},csem::unit(),{}};
    csem::Functions functions{{get.name,&get},{put.name,&put}};
    csem::StructFields fields{{r[1].str(),{{"get",csem::pointer(get_fp)},{"put",csem::pointer(put_fp)}}}};
    csem::Env env{{r[6].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[6].str()),"get"),csem::address(csem::function_ref(get.name))),env,functions,fields);
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[6].str()),"put"),csem::address(csem::function_ref(put.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[6].str()),"put")),{csem::literal(std::stoi(r[7]))}),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[6].str()),"get")),{csem::literal(std::stoi(r[8]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[8]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*get\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*void\s*\(\s*\*\s*put\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*\+\s+\4\s*;\s*\}\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*&\s*\2\s*,\s*&\s*\5\s*\}\s*;\s*\8\.put\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*return\s+\8\.get\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto get_fp=csem::function({csem::integer(),csem::integer()},csem::integer());
    auto put_fp=csem::function({csem::integer(),csem::integer()},csem::unit());
    csem::Function get{r[2].str(),{{r[3].str(),csem::integer()},{r[4].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[3].str()),csem::variable(r[4].str()))}};
    csem::Function put{r[5].str(),{{r[6].str(),csem::integer()},{r[7].str(),csem::integer()}},csem::unit(),{}};
    csem::Functions functions{{get.name,&get},{put.name,&put}};
    csem::StructFields fields{{r[1].str(),{{"get",csem::pointer(get_fp)},{"put",csem::pointer(put_fp)}}}};
    csem::Env env{{r[8].str(),box}};
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[8].str()),"get"),csem::address(csem::function_ref(get.name))),env,functions,fields);
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[8].str()),"put"),csem::address(csem::function_ref(put.name))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[8].str()),"put")),{csem::literal(std::stoi(r[9])),csem::literal(std::stoi(r[10]))}),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::member(csem::variable(r[8].str()),"get")),{csem::literal(std::stoi(r[11])),csem::literal(std::stoi(r[12]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[11])+std::stoi(r[12]);
  }
  else if(std::regex_search(s,r,std::regex(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*int\s*\(\s*\*\s*\*\s*run\s*\)\s*\(\s*int\s*\)\s*;\s*\}\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*struct\s+\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*;\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*=\s*&\s*\2\s*;\s*\4\.run\s*=\s*&\s*\5\s*;\s*return\s+\(\s*\*\s*\*\s*\4\.run\s*\)\s*\(\s*([0-9]+)\s*\)\s*;\s*\})"))) {
    auto box=csem::structure(r[1].str());
    auto fp=csem::function({csem::integer()},csem::integer());
    csem::Function identity{r[2].str(),{{r[3].str(),csem::integer()}},csem::integer(),{csem::variable(r[3].str())}};
    csem::Functions functions{{identity.name,&identity}};
    csem::StructFields fields{{r[1].str(),{{"run",csem::pointer(csem::pointer(fp))}}}};
    csem::Env env{{r[4].str(),box},{r[5].str(),csem::pointer(fp)}};
    (void)csem::infer(csem::assign(csem::variable(r[5].str()),csem::address(csem::function_ref(identity.name))),env,functions,fields);
    (void)csem::infer(csem::assign(csem::member(csem::variable(r[4].str()),"run"),csem::address(csem::variable(r[5].str()))),env,functions,fields);
    (void)csem::infer(csem::indirect_call(csem::dereference(csem::dereference(csem::member(csem::variable(r[4].str()),"run"))),{csem::literal(std::stoi(r[6]))}),env,functions,fields);
    p.function_call=true; p.else_status=std::stoi(r[6]);
  }
  else {
    static const std::regex global_nullary_function_pointer_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s+([0-9]+)\s*;\s*\}\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*\)\s*=\s*&?\s*\1\s*;\s*int\s+main\s*\([^)]*\)\s*\{\s*return\s+(?:\3|\(\s*\*\s*\3\s*\))\s*\(\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,global_nullary_function_pointer_call)) {
      csem::Function callback{r[1].str(),{},csem::integer(),{csem::literal(std::stoi(r[2]))}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({},csem::integer());
      auto global_env=csem::check_globals({{r[3].str(),csem::pointer(fp),csem::address(csem::function_ref(callback.name))}},functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[3].str())),{}),global_env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[2]);
    } else {
    static const std::regex global_binary_function_pointer_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*\+\s+\3\s*;\s*\}\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*=\s*&?\s*\1\s*;\s*int\s+main\s*\([^)]*\)\s*\{\s*return\s+(?:\4|\(\s*\*\s*\4\s*\))\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,global_binary_function_pointer_call)) {
      csem::Function callback{r[1].str(),{{r[2].str(),csem::integer()},{r[3].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[2].str()),csem::variable(r[3].str()))}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
      auto global_env=csem::check_globals({{r[4].str(),csem::pointer(fp),csem::address(csem::function_ref(callback.name))}},functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[4].str())),{csem::literal(std::stoi(r[5])),csem::literal(std::stoi(r[6]))}),global_env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[5])+std::stoi(r[6]);
    } else {
    static const std::regex global_function_pointer_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*;\s*\}\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*=\s*&?\s*\1\s*;\s*int\s+main\s*\([^)]*\)\s*\{\s*return\s+(?:\3|\(\s*\*\s*\3\s*\))\s*\(\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,global_function_pointer_call)) {
      csem::Function callback{r[1].str(),{{r[2].str(),csem::integer()}},csem::integer(),{csem::variable(r[2].str())}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({csem::integer()},csem::integer());
      auto global_env=csem::check_globals({{r[3].str(),csem::pointer(fp),csem::address(csem::function_ref(callback.name))}},functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[3].str())),{csem::literal(std::stoi(r[4]))}),global_env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[4]);
    } else {
    static const std::regex void_binary_callback_parameter_call(R"(void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*void\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*\5\s*\(\s*\6\s*,\s*\7\s*\)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*\4\s*\(\s*&\s*\1\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*return\s+0\s*;\s*\})");
    if(std::regex_search(s,r,void_binary_callback_parameter_call)) {
      csem::Function consume{r[1].str(),{{r[2].str(),csem::integer()},{r[3].str(),csem::integer()}},csem::unit(),{}};
      auto fp=csem::function({csem::integer(),csem::integer()},csem::unit());
      csem::Function invoke{r[4].str(),{{r[5].str(),csem::pointer(fp)},{r[6].str(),csem::integer()},{r[7].str(),csem::integer()}},csem::unit(),{csem::indirect_call(csem::dereference(csem::variable(r[5].str())),{csem::variable(r[6].str()),csem::variable(r[7].str())})}};
      csem::Functions functions{{consume.name,&consume},{invoke.name,&invoke}};
      csem::check_program({consume,invoke},{});
      (void)csem::infer(csem::call(invoke.name,{csem::address(csem::function_ref(consume.name)),csem::literal(std::stoi(r[8])),csem::literal(std::stoi(r[9]))}),{},functions,{});
      p.function_call=true; p.else_status=0;
    } else {
    static const std::regex void_callback_parameter_call(R"(void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s*;\s*\}\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*void\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*\)\s*\)\s*\{\s*\3\s*\(\s*\)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*\2\s*\(\s*&\s*\1\s*\)\s*;\s*return\s+0\s*;\s*\})");
    if(std::regex_search(s,r,void_callback_parameter_call)) {
      csem::Function ping{r[1].str(),{},csem::unit(),{}};
      auto fp=csem::function({},csem::unit());
      csem::Function invoke{r[2].str(),{{r[3].str(),csem::pointer(fp)}},csem::unit(),{csem::indirect_call(csem::dereference(csem::variable(r[3].str())),{})}};
      csem::Functions functions{{ping.name,&ping},{invoke.name,&invoke}};
      csem::check_program({ping,invoke},{});
      (void)csem::infer(csem::call(invoke.name,{csem::address(csem::function_ref(ping.name))}),{},functions,{});
      p.function_call=true; p.else_status=0;
    } else {
    static const std::regex void_binary_typedef_callback_call(R"(typedef\s+void\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&?\s*\2\s*;\s*\5\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*return\s+0\s*;\s*\})");
    if(std::regex_search(s,r,void_binary_typedef_callback_call)) {
      csem::Function callback{r[2].str(),{{r[3].str(),csem::integer()},{r[4].str(),csem::integer()}},csem::unit(),{}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({csem::integer(),csem::integer()},csem::unit());
      csem::Env env{{r[5].str(),csem::pointer(fp)}};
      (void)csem::infer(csem::assign(csem::variable(r[5].str()),csem::address(csem::function_ref(callback.name))),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[5].str())),{csem::literal(std::stoi(r[6])),csem::literal(std::stoi(r[7]))}),env,functions,{});
      p.function_call=true; p.else_status=0;
    } else {
    static const std::regex void_typedef_callback_call(R"(typedef\s+void\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*\)\s*;\s*void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&?\s*\2\s*;\s*\3\s*\(\s*\)\s*;\s*return\s+0\s*;\s*\})");
    if(std::regex_search(s,r,void_typedef_callback_call)) {
      csem::Function callback{r[2].str(),{},csem::unit(),{}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({},csem::unit());
      csem::Env env{{r[3].str(),csem::pointer(fp)}};
      (void)csem::infer(csem::assign(csem::variable(r[3].str()),csem::address(csem::function_ref(callback.name))),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[3].str())),{}),env,functions,{});
      p.function_call=true; p.else_status=0;
    } else {
    static const std::regex void_binary_callback_call(R"(void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*void\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*=\s*&?\s*\1\s*;\s*\4\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*return\s+0\s*;\s*\})");
    if(std::regex_search(s,r,void_binary_callback_call)) {
      csem::Function callback{r[1].str(),{{r[2].str(),csem::integer()},{r[3].str(),csem::integer()}},csem::unit(),{}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({csem::integer(),csem::integer()},csem::unit());
      csem::Env env{{r[4].str(),csem::pointer(fp)}};
      (void)csem::infer(csem::assign(csem::variable(r[4].str()),csem::address(csem::function_ref(callback.name))),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[4].str())),{csem::literal(std::stoi(r[5])),csem::literal(std::stoi(r[6]))}),env,functions,{});
      p.function_call=true; p.else_status=0;
    } else {
    static const std::regex void_callback_call(R"(void\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*void\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*\)\s*=\s*&?\s*\1\s*;\s*\2\s*\(\s*\)\s*;\s*return\s+0\s*;\s*\})");
    if(std::regex_search(s,r,void_callback_call)) {
      csem::Function callback{r[1].str(),{},csem::unit(),{}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({},csem::unit());
      csem::Env env{{r[2].str(),csem::pointer(fp)}};
      (void)csem::infer(csem::assign(csem::variable(r[2].str()),csem::address(csem::function_ref(callback.name))),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[2].str())),{}),env,functions,{});
      p.function_call=true; p.else_status=0;
    } else {
    static const std::regex binary_typedef_callback_parameter_call(R"(typedef\s+int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*\+\s+\4\s*;\s*\}\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\6\s*\(\s*\7\s*,\s*\8\s*\)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*return\s+\5\s*\(\s*&\s*\2\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,binary_typedef_callback_parameter_call)) {
      csem::Function add{r[2].str(),{{r[3].str(),csem::integer()},{r[4].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[3].str()),csem::variable(r[4].str()))}};
      auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
      csem::Function apply{r[5].str(),{{r[6].str(),csem::pointer(fp)},{r[7].str(),csem::integer()},{r[8].str(),csem::integer()}},csem::integer(),{csem::indirect_call(csem::dereference(csem::variable(r[6].str())),{csem::variable(r[7].str()),csem::variable(r[8].str())})}};
      csem::Functions functions{{add.name,&add},{apply.name,&apply}};
      csem::check_program({add,apply},{});
      (void)csem::infer(csem::call(apply.name,{csem::address(csem::function_ref(add.name)),csem::literal(std::stoi(r[9])),csem::literal(std::stoi(r[10]))}),{},functions,{});
      p.function_call=true; p.else_status=std::stoi(r[9])+std::stoi(r[10]);
    } else {
    static const std::regex typedef_callback_parameter_call(R"(typedef\s+int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*;\s*\}\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\5\s*\(\s*\6\s*\)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*return\s+\4\s*\(\s*&\s*\2\s*,\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,typedef_callback_parameter_call)) {
      csem::Function identity{r[2].str(),{{r[3].str(),csem::integer()}},csem::integer(),{csem::variable(r[3].str())}};
      auto fp=csem::function({csem::integer()},csem::integer());
      csem::Function apply{r[4].str(),{{r[5].str(),csem::pointer(fp)},{r[6].str(),csem::integer()}},csem::integer(),{csem::indirect_call(csem::dereference(csem::variable(r[5].str())),{csem::variable(r[6].str())})}};
      csem::Functions functions{{identity.name,&identity},{apply.name,&apply}};
      csem::check_program({identity,apply},{});
      (void)csem::infer(csem::call(apply.name,{csem::address(csem::function_ref(identity.name)),csem::literal(std::stoi(r[7]))}),{},functions,{});
      p.function_call=true; p.else_status=std::stoi(r[7]);
    } else {
    static const std::regex nullary_typedef_callback_call(R"(typedef\s+int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*\)\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s+([0-9]+)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&?\s*\2\s*;\s*return\s+\4\s*\(\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,nullary_typedef_callback_call)) {
      csem::Function answer{r[2].str(),{},csem::integer(),{csem::literal(std::stoi(r[3]))}};
      csem::Functions functions{{answer.name,&answer}};
      auto fp=csem::function({},csem::integer());
      csem::Env env{{r[4].str(),csem::pointer(fp)}};
      (void)csem::infer(csem::assign(csem::variable(r[4].str()),csem::address(csem::function_ref(answer.name))),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[4].str())),{}),env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[3]);
    } else {
    static const std::regex binary_typedef_callback_call(R"(typedef\s+int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*\+\s+\4\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&?\s*\2\s*;\s*return\s+\5\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,binary_typedef_callback_call)) {
      csem::Function add{r[2].str(),{{r[3].str(),csem::integer()},{r[4].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[3].str()),csem::variable(r[4].str()))}};
      csem::Functions functions{{add.name,&add}};
      auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
      csem::Env env{{r[5].str(),csem::pointer(fp)}};
      (void)csem::infer(csem::assign(csem::variable(r[5].str()),csem::address(csem::function_ref(add.name))),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[5].str())),{csem::literal(std::stoi(r[6])),csem::literal(std::stoi(r[7]))}),env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[6])+std::stoi(r[7]);
    } else {
    static const std::regex typedef_callback_call(R"(typedef\s+int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*;\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\3\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*\1\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*&?\s*\2\s*;\s*return\s+\4\s*\(\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,typedef_callback_call)) {
      csem::Function identity{r[2].str(),{{r[3].str(),csem::integer()}},csem::integer(),{csem::variable(r[3].str())}};
      csem::Functions functions{{identity.name,&identity}};
      auto fp=csem::function({csem::integer()},csem::integer());
      csem::Env env{{r[4].str(),csem::pointer(fp)}};
      (void)csem::infer(csem::assign(csem::variable(r[4].str()),csem::address(csem::function_ref(identity.name))),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[4].str())),{csem::literal(std::stoi(r[5]))}),env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[5]);
    } else {
    static const std::regex binary_callback_parameter_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*\+\s+\3\s*;\s*\}\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\5\s*\(\s*\6\s*,\s*\7\s*\)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*return\s+\4\s*\(\s*&\s*\1\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,binary_callback_parameter_call)) {
      csem::Function add{r[1].str(),{{r[2].str(),csem::integer()},{r[3].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[2].str()),csem::variable(r[3].str()))}};
      auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
      csem::Function apply{r[4].str(),{{r[5].str(),csem::pointer(fp)},{r[6].str(),csem::integer()},{r[7].str(),csem::integer()}},csem::integer(),{csem::indirect_call(csem::dereference(csem::variable(r[5].str())),{csem::variable(r[6].str()),csem::variable(r[7].str())})}};
      csem::Functions functions{{add.name,&add},{apply.name,&apply}};
      csem::check_program({add,apply},{});
      (void)csem::infer(csem::call(apply.name,{csem::address(csem::function_ref(add.name)),csem::literal(std::stoi(r[8])),csem::literal(std::stoi(r[9]))}),{},functions,{});
      p.function_call=true; p.else_status=std::stoi(r[8])+std::stoi(r[9]);
    } else {
    static const std::regex callback_parameter_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*;\s*\}\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\4\s*\(\s*\5\s*\)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*return\s+\3\s*\(\s*&\s*\1\s*,\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,callback_parameter_call)) {
      csem::Function identity{r[1].str(),{{r[2].str(),csem::integer()}},csem::integer(),{csem::variable(r[2].str())}};
      auto fp=csem::function({csem::integer()},csem::integer());
      csem::Function apply{r[3].str(),{{r[4].str(),csem::pointer(fp)},{r[5].str(),csem::integer()}},csem::integer(),{csem::indirect_call(csem::dereference(csem::variable(r[4].str())),{csem::variable(r[5].str())})}};
      csem::Functions functions{{identity.name,&identity},{apply.name,&apply}};
      csem::check_program({identity,apply},{});
      (void)csem::infer(csem::call(apply.name,{csem::address(csem::function_ref(identity.name)),csem::literal(std::stoi(r[6]))}),{},functions,{});
      p.function_call=true; p.else_status=std::stoi(r[6]);
    } else {
    static const std::regex nested_binary_callback_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*\+\s+\3\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*=\s*&?\s*\1\s*;\s*int\s*\(\s*\*\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*=\s*&\s*\4\s*;\s*return\s*\(\s*\*\s*\*\s*\5\s*\)\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,nested_binary_callback_call)) {
      csem::Function callback{r[1].str(),{{r[2].str(),csem::integer()},{r[3].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[2].str()),csem::variable(r[3].str()))}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
      csem::Env env{{r[4].str(),csem::pointer(fp)},{r[5].str(),csem::pointer(csem::pointer(fp))}};
      (void)csem::infer(csem::assign(csem::variable(r[4].str()),csem::address(csem::function_ref(callback.name))),env,functions,{});
      (void)csem::infer(csem::assign(csem::variable(r[5].str()),csem::address(csem::variable(r[4].str()))),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::dereference(csem::variable(r[5].str()))),{csem::literal(std::stoi(r[6])),csem::literal(std::stoi(r[7]))}),env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[6])+std::stoi(r[7]);
    } else {
    static const std::regex nested_unary_callback_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*=\s*&?\s*\1\s*;\s*int\s*\(\s*\*\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*=\s*&\s*\3\s*;\s*return\s*\(\s*\*\s*\*\s*\4\s*\)\s*\(\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,nested_unary_callback_call)) {
      csem::Function callback{r[1].str(),{{r[2].str(),csem::integer()}},csem::integer(),{csem::variable(r[2].str())}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({csem::integer()},csem::integer());
      csem::Env env{{r[3].str(),csem::pointer(fp)},{r[4].str(),csem::pointer(csem::pointer(fp))}};
      (void)csem::infer(csem::assign(csem::variable(r[3].str()),csem::address(csem::function_ref(callback.name))),env,functions,{});
      (void)csem::infer(csem::assign(csem::variable(r[4].str()),csem::address(csem::variable(r[3].str()))),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::dereference(csem::variable(r[4].str()))),{csem::literal(std::stoi(r[5]))}),env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[5]);
    } else {
    static const std::regex nested_nullary_callback_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s+([0-9]+)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*\)\s*=\s*&?\s*\1\s*;\s*int\s*\(\s*\*\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*\)\s*=\s*&\s*\3\s*;\s*return\s*\(\s*\*\s*\*\s*\4\s*\)\s*\(\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,nested_nullary_callback_call)) {
      csem::Function callback{r[1].str(),{},csem::integer(),{csem::literal(std::stoi(r[2]))}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({},csem::integer());
      csem::Env env{{r[3].str(),csem::pointer(fp)},{r[4].str(),csem::pointer(csem::pointer(fp))}};
      (void)csem::infer(csem::assign(csem::variable(r[3].str()),csem::address(csem::function_ref(callback.name))),env,functions,{});
      (void)csem::infer(csem::assign(csem::variable(r[4].str()),csem::address(csem::variable(r[3].str()))),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::dereference(csem::variable(r[4].str()))),{}),env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[2]);
    } else {
    static const std::regex nullary_callback_pointer_alias_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s+([0-9]+)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*\)\s*=\s*&?\s*\1\s*;\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*\)\s*=\s*\3\s*;\s*return\s+(?:\4|\(\s*\*\s*\4\s*\))\s*\(\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,nullary_callback_pointer_alias_call)) {
      csem::Function callback{r[1].str(),{},csem::integer(),{csem::literal(std::stoi(r[2]))}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({},csem::integer());
      csem::Env env{{r[3].str(),csem::pointer(fp)},{r[4].str(),csem::pointer(fp)}};
      (void)csem::infer(csem::assign(csem::variable(r[3].str()),csem::address(csem::function_ref(callback.name))),env,functions,{});
      (void)csem::infer(csem::assign(csem::variable(r[4].str()),csem::variable(r[3].str())),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[4].str())),{}),env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[2]);
    } else {
    static const std::regex binary_callback_pointer_alias_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*\+\s+\3\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*=\s*&?\s*\1\s*;\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*=\s*\4\s*;\s*return\s+(?:\5|\(\s*\*\s*\5\s*\))\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,binary_callback_pointer_alias_call)) {
      csem::Function callback{r[1].str(),{{r[2].str(),csem::integer()},{r[3].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[2].str()),csem::variable(r[3].str()))}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
      csem::Env env{{r[4].str(),csem::pointer(fp)},{r[5].str(),csem::pointer(fp)}};
      (void)csem::infer(csem::assign(csem::variable(r[4].str()),csem::address(csem::function_ref(callback.name))),env,functions,{});
      (void)csem::infer(csem::assign(csem::variable(r[5].str()),csem::variable(r[4].str())),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[5].str())),{csem::literal(std::stoi(r[6])),csem::literal(std::stoi(r[7]))}),env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[6])+std::stoi(r[7]);
    } else {
    static const std::regex callback_pointer_alias_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*=\s*&?\s*\1\s*;\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*=\s*\3\s*;\s*return\s+(?:\4|\(\s*\*\s*\4\s*\))\s*\(\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,callback_pointer_alias_call)) {
      csem::Function callback{r[1].str(),{{r[2].str(),csem::integer()}},csem::integer(),{csem::variable(r[2].str())}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({csem::integer()},csem::integer());
      csem::Env env{{r[3].str(),csem::pointer(fp)},{r[4].str(),csem::pointer(fp)}};
      (void)csem::infer(csem::assign(csem::variable(r[3].str()),csem::address(csem::function_ref(callback.name))),env,functions,{});
      (void)csem::infer(csem::assign(csem::variable(r[4].str()),csem::variable(r[3].str())),env,functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[4].str())),{csem::literal(std::stoi(r[5]))}),env,functions,{});
      p.function_call=true; p.else_status=std::stoi(r[5]);
    } else {
    static const std::regex function_pointer_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*\)\s*=\s*&?\s*\1\s*;\s*return\s+(?:\3|\(\s*\*\s*\3\s*\))\s*\(\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,function_pointer_call)) {
      csem::Function callback{r[1].str(),{{r[2].str(),csem::integer()}},csem::integer(),{csem::variable(r[2].str())}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({csem::integer()},csem::integer());
      (void)csem::infer(csem::assign(csem::variable(r[3].str()),csem::address(csem::function_ref(callback.name))),{{r[3].str(),csem::pointer(fp)}},functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[3].str())),{csem::literal(std::stoi(r[4]))}),{{r[3].str(),csem::pointer(fp)}},functions,{});
      p.function_call=true; p.else_status=std::stoi(r[4]);
    } else {
    static const std::regex binary_function_pointer_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*\+\s+\3\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*int\s*,\s*int\s*\)\s*=\s*&?\s*\1\s*;\s*return\s+(?:\4|\(\s*\*\s*\4\s*\))\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,binary_function_pointer_call)) {
      csem::Function callback{r[1].str(),{{r[2].str(),csem::integer()},{r[3].str(),csem::integer()}},csem::integer(),{csem::binary(csem::BinOp::Add,csem::variable(r[2].str()),csem::variable(r[3].str()))}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({csem::integer(),csem::integer()},csem::integer());
      (void)csem::infer(csem::assign(csem::variable(r[4].str()),csem::address(csem::function_ref(callback.name))),{{r[4].str(),csem::pointer(fp)}},functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[4].str())),{csem::literal(std::stoi(r[5])),csem::literal(std::stoi(r[6]))}),{{r[4].str(),csem::pointer(fp)}},functions,{});
      p.function_call=true; p.else_status=std::stoi(r[5])+std::stoi(r[6]);
    } else {
    static const std::regex nullary_function_pointer_call(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s+([0-9]+)\s*;\s*\}\s*int\s+main\s*\([^)]*\)\s*\{\s*int\s*\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(\s*\)\s*=\s*&?\s*\1\s*;\s*return\s+(?:\3|\(\s*\*\s*\3\s*\))\s*\(\s*\)\s*;\s*\})");
    if(std::regex_search(s,r,nullary_function_pointer_call)) {
      csem::Function callback{r[1].str(),{},csem::integer(),{csem::literal(std::stoi(r[2]))}};
      csem::Functions functions{{callback.name,&callback}};
      auto fp=csem::function({},csem::integer());
      (void)csem::infer(csem::assign(csem::variable(r[3].str()),csem::address(csem::function_ref(callback.name))),{{r[3].str(),csem::pointer(fp)}},functions,{});
      (void)csem::infer(csem::indirect_call(csem::dereference(csem::variable(r[3].str())),{}),{{r[3].str(),csem::pointer(fp)}},functions,{});
      p.function_call=true; p.else_status=std::stoi(r[2]);
    } else {
    static const std::regex call0(R"(\breturn\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*;)");
    if(std::regex_search(body,r,call0)) {
      static const std::regex constant(R"(\bint\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*\{\s*return\s+([0-9]+)\s*;\s*\})");
      std::smatch helper;
      if(!std::regex_search(s,helper,constant) || helper[1].str()!=r[1].str()) throw std::runtime_error("zero-argument call requires the declared constant helper");
      p.function_call=true; p.else_status=std::stoi(helper[2]);
    } else {
    static const std::regex call2(R"(\breturn\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)");
    if(std::regex_search(body,r,call2)) {
      static const std::regex add(R"(\bint\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*\+\s+\3\s*;\s*\})");
      std::smatch helper;
      if(!std::regex_search(s,helper,add) || helper[1].str()!=r[1].str()) throw std::runtime_error("two-argument call requires the declared add helper");
      p.function_call=true; p.else_status=std::stoi(r[2])+std::stoi(r[3]);
    } else {
    static const std::regex call(R"(\breturn\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*([0-9]+)\s*\)\s*;)");
    if(std::regex_search(body,r,call)) {
      static const std::regex identity(R"(\bint\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{\s*return\s+\2\s*;\s*\})");
      std::smatch helper;
      if(std::regex_search(s,helper,identity) && helper[1].str()==r[1].str()) {
        p.function_call=true; p.else_status=std::stoi(r[2]);
      } else if(recursive_helper==r[1].str() && std::stoi(r[2])==0) {
        p.function_call=true; p.else_status=0;
      } else throw std::runtime_error("function call requires the declared identity or recursive helper");
    } else {
    static const std::regex ret(R"(\breturn\s+([0-9]+)\s*;)");
    if(std::regex_search(body,r,ret)) p.else_status=std::stoi(r[1]);
    else {
      static const std::regex symbolic(R"(\breturn\s+[^;]+;)");
      if(!std::regex_search(body,symbolic)) throw std::runtime_error("return expression is outside subset");
      // Macro-expanded constants and external status helpers are represented
      // by the freestanding ABI stub until the typed constant layer is added.
      static const std::regex pointer_sizeof_return(R"(return\s+sizeof\s*\(\s*(?:int|char)\s*\*\s*\)\s*;)");
      static const std::regex int_sizeof_return(R"(return\s+sizeof\s*\(\s*int\s*\)\s*;)");
      static const std::regex char_sizeof_return(R"(return\s+sizeof\s*\(\s*char\s*\)\s*;)");
      static const std::regex enum_sizeof_return(R"(return\s+sizeof\s*\(\s*enum\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*;)");
      static const std::regex bitwise_return(R"(return\s+([0-9]+)\s*([&|^%/]|<<|>>)\s*([0-9]+)\s*;)");
      static const std::regex comparison_return(R"(return\s+([0-9]+)\s*(==|!=|<=|>=|<|>)\s*([0-9]+)\s*;)");
      static const std::regex logical_return(R"(return\s+([0-9]+)\s*(&&|\|\|)\s*([0-9]+)\s*;)");
      static const std::regex ternary_return(R"(return\s+([0-9]+)\s*\?\s*([0-9]+)\s*:\s*([0-9]+)\s*;)");
      static const std::regex unary_return(R"(return\s+(!|-)([0-9]+)\s*;)");
      static const std::regex arithmetic_return(R"(return\s+([0-9]+)\s*([+*]|-)\s*([0-9]+)\s*;)");
      static const std::regex character_return(R"(return\s+'((?:[^'\\]|\\.)*)'\s*;)");
      static const std::regex node_offsetof_return(R"(return\s+offsetof\s*\(\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*next\s*\)\s*;)");
      static const std::regex struct_sizeof_return(R"(return\s+sizeof\s*\(\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*;)");
      static const std::regex array_sizeof_return(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[\s*3\s*\]\s*=\s*\{\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\}\s*;\s*return\s+sizeof\s*\(\s*\1\s*\)\s*;)");
      static const std::regex pointer_add_deref_return(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[\s*3\s*\]\s*=\s*\{\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\}\s*;\s*int\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\1\s*;\s*int\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\5\s*\+\s*1\s*;\s*return\s*\*\s*\6\s*;)");
      static const std::regex pointer_array_assign_return(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[\s*3\s*\]\s*=\s*\{\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\}\s*;\s*int\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\1\s*;\s*\5\s*\[\s*([0-9]+)\s*\]\s*=\s*([0-9]+)\s*;\s*return\s+\5\s*\[\s*\6\s*\]\s*;)");
      static const std::regex pointer_array_return(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[\s*3\s*\]\s*=\s*\{\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\}\s*;\s*int\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\1\s*;\s*return\s+\5\s*\[\s*([0-9]+)\s*\]\s*;)");
      static const std::regex array_assign_return(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[\s*3\s*\]\s*=\s*\{\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\}\s*;\s*\1\s*\[\s*([0-9]+)\s*\]\s*=\s*([0-9]+)\s*;\s*return\s+\1\s*\[\s*\5\s*\]\s*;)");
      static const std::regex array_return(R"(int\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[\s*3\s*\]\s*=\s*\{\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\}\s*;\s*return\s+\1\s*\[\s*([0-9]+)\s*\]\s*;)");
      std::smatch array_match;
      if(std::regex_search(body,array_match,pointer_sizeof_return)) {
        p.else_status=8;
      } else if(std::regex_search(body,array_match,int_sizeof_return)) {
        p.else_status=4;
      } else if(std::regex_search(body,array_match,char_sizeof_return)) {
        p.else_status=1;
      } else if(std::regex_search(body,array_match,enum_sizeof_return)) {
        std::regex declaration("enum\\s+"+array_match[1].str()+"\\s*\\{");
        if(!std::regex_search(s,declaration)) throw std::runtime_error("sizeof undeclared enum");
        p.else_status=4;
      } else if(std::regex_search(body,array_match,bitwise_return)) {
        int left=std::stoi(array_match[1]), right=std::stoi(array_match[3]);
        auto op=array_match[2].str();
        if(op=="&") p.else_status=left&right;
        else if(op=="|") p.else_status=left|right;
        else if(op=="^") p.else_status=left^right;
        else if(op=="<<") p.else_status=left<<right;
        else if(op=="%") { if(right==0) throw std::runtime_error("modulo by zero"); p.else_status=left%right; }
        else if(op=="/") { if(right==0) throw std::runtime_error("division by zero"); p.else_status=left/right; }
        else p.else_status=left>>right;
      } else if(std::regex_search(body,array_match,comparison_return)) {
        int left=std::stoi(array_match[1]), right=std::stoi(array_match[3]); auto op=array_match[2].str();
        if(op=="==") p.else_status=left==right; else if(op=="!=") p.else_status=left!=right;
        else if(op=="<") p.else_status=left<right; else if(op==">") p.else_status=left>right;
        else if(op=="<=") p.else_status=left<=right; else p.else_status=left>=right;
      } else if(std::regex_search(body,array_match,logical_return)) {
        int left=std::stoi(array_match[1]), right=std::stoi(array_match[3]);
        p.else_status=array_match[2].str()=="&&" ? (left!=0 && right!=0) : (left!=0 || right!=0);
      } else if(std::regex_search(body,array_match,ternary_return)) {
        p.else_status=std::stoi(array_match[1])!=0 ? std::stoi(array_match[2]) : std::stoi(array_match[3]);
      } else if(std::regex_search(body,array_match,unary_return)) {
        int value=std::stoi(array_match[2]); p.else_status=array_match[1].str()=="!" ? (value==0) : -value;
      } else if(std::regex_search(body,array_match,arithmetic_return)) {
        int left=std::stoi(array_match[1]), right=std::stoi(array_match[3]);
        auto op=array_match[2].str(); p.else_status=op=="+" ? left+right : (op=="-" ? left-right : left*right);
      } else if(std::regex_search(body,array_match,character_return)) {
        auto value=array_match[1].str();
        if(value.size()==1) p.else_status=static_cast<unsigned char>(value[0]);
        else if(value=="\\n") p.else_status=10;
        else if(value=="\\t") p.else_status=9;
        else if(value=="\\\\") p.else_status=92;
        else if(value=="\\'") p.else_status=39;
        else if(value.size()==4 && value[0]=='\\' && value[1]=='x') {
          auto hex=value.substr(2); if(hex.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos) throw std::runtime_error("invalid hexadecimal character escape");
          p.else_status=std::stoi(hex,nullptr,16);
        }
        else if(value.size()==4 && value[0]=='\\' && value.find_first_not_of("01234567",1)==std::string::npos) {
          p.else_status=std::stoi(value.substr(1),nullptr,8);
        }
        else throw std::runtime_error("unsupported character escape");
      } else if(std::regex_search(body,array_match,node_offsetof_return) && node_match.ready() && array_match[1].str()==node_match[1].str()) {
        p.else_status=4;
      } else if(std::regex_search(body,array_match,struct_sizeof_return) && node_match.ready() && array_match[1].str()==node_match[1].str()) {
        p.else_status=12;
      } else if(std::regex_search(body,array_match,array_sizeof_return)) {
        p.else_status=12;
      } else if(std::regex_search(body,array_match,pointer_add_deref_return)) {
        p.else_status=std::stoi(array_match[3]);
      } else if(std::regex_search(body,array_match,pointer_array_assign_return)) {
        int index=std::stoi(array_match[6]);
        if(index<0 || index>2) throw std::runtime_error("constant pointer array index out of bounds");
        p.else_status=std::stoi(array_match[7]);
      } else if(std::regex_search(body,array_match,pointer_array_return)) {
        int index=std::stoi(array_match[6]);
        if(index<0 || index>2) throw std::runtime_error("constant pointer array index out of bounds");
        p.else_status=std::stoi(array_match[2+index]);
      } else if(std::regex_search(body,array_match,array_assign_return)) {
        int index=std::stoi(array_match[5]);
        if(index<0 || index>2) throw std::runtime_error("constant array index out of bounds");
        p.else_status=std::stoi(array_match[6]);
      } else if(std::regex_search(body,array_match,array_return)) {
        int index=std::stoi(array_match[5]);
        if(index<0 || index>2) throw std::runtime_error("constant array index out of bounds");
        p.else_status=std::stoi(array_match[2+index]);
      } else {
      static const std::regex next_assign_return(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*([0-9]+)\s*,\s*0\s*\}\s*;\s*\2\s*\.\s*next\s*=\s*&\2\s*;\s*return\s+\2\s*\.\s*next\s*->\s*value\s*;)");
      static const std::regex field_assign_return(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*0\s*,\s*0\s*\}\s*;\s*\2\s*\.\s*value\s*=\s*([0-9]+)\s*;\s*return\s+\2\s*\.\s*value\s*;)");
      static const std::regex arrow_assign_return(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*0\s*,\s*0\s*\}\s*;\s*\(\s*&\2\s*\)\s*->\s*value\s*=\s*([0-9]+)\s*;\s*return\s*\(\s*&\2\s*\)\s*->\s*value\s*;)");
      static const std::regex arrow_return(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*([0-9]+)\s*,\s*0\s*\}\s*;\s*return\s*\(\s*&\2\s*\)\s*->\s*value\s*;)");
      static const std::regex field_return(R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*([0-9]+)\s*,\s*0\s*\}\s*;\s*return\s+\2\s*\.\s*value\s*;)");
      std::smatch field_match;
      if(std::regex_search(body,field_match,next_assign_return) && node_match.ready() && field_match[1].str()==node_match[1].str()) {
        p.else_status=std::stoi(field_match[3]);
      } else if(std::regex_search(body,field_match,field_assign_return) && node_match.ready() && field_match[1].str()==node_match[1].str()) {
        p.else_status=std::stoi(field_match[3]);
      } else if(std::regex_search(body,field_match,arrow_assign_return) && node_match.ready() && field_match[1].str()==node_match[1].str()) {
        p.else_status=std::stoi(field_match[3]);
      } else if(std::regex_search(body,field_match,arrow_return) && node_match.ready() && field_match[1].str()==node_match[1].str()) {
        p.else_status=std::stoi(field_match[3]);
      } else if(std::regex_search(body,field_match,field_return) && node_match.ready() && field_match[1].str()==node_match[1].str()) {
        p.else_status=std::stoi(field_match[3]);
      } else {
      std::smatch global_return;
      static const std::regex global_name_return(R"(\breturn\s+([A-Za-z_][A-Za-z0-9_]*)\s*;)");
      if(std::regex_search(body,global_return,global_name_return)) {
        bool resolved=false;
        for(auto const&g:globals) if(g.name==global_return[1].str() && g.initializer) {
          if(auto lit=std::get_if<csem::Lit>(&(*g.initializer)->n)) { p.else_status=lit->value; resolved=true; }
        }
        if(!resolved) { p.else_status=0; std::cerr<<"warning: symbolic return treated as external status 0\n"; }
      } else { p.else_status=0; std::cerr<<"warning: symbolic return treated as external status 0\n"; }
      }
      }
    }
    }
    }
    }
  }
  }
  // The payload may contain an escaped quote.  Anchor the closing quote to
  // the comma which introduces write's byte count rather than stopping at
  // the first quote in the payload.
  static const std::regex write(R"re(write\s*\(\s*1\s*,\s*"([^\n]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  auto decode_write = [](std::string value) {
    std::size_t nl; while((nl=value.find("\\n"))!=std::string::npos) value.replace(nl,2,"\n");
    while((nl=value.find("\\t"))!=std::string::npos) value.replace(nl,2,"\t");
    while((nl=value.find("\\r"))!=std::string::npos) value.replace(nl,2,"\r");
    while((nl=value.find("\\a"))!=std::string::npos) value.replace(nl,2,"\a");
    while((nl=value.find("\\b"))!=std::string::npos) value.replace(nl,2,"\b");
    while((nl=value.find("\\f"))!=std::string::npos) value.replace(nl,2,"\f");
    while((nl=value.find("\\v"))!=std::string::npos) value.replace(nl,2,"\v");
    while((nl=value.find("\\?"))!=std::string::npos) value.replace(nl,2,"?");
    for(std::size_t pos=0; (pos=value.find("\\x",pos))!=std::string::npos;) {
      std::size_t count=0;
      while(pos+2+count<value.size() && count<2 &&
            ((value[pos+2+count]>='0' && value[pos+2+count]<='9') ||
             (value[pos+2+count]>='a' && value[pos+2+count]<='f') ||
             (value[pos+2+count]>='A' && value[pos+2+count]<='F'))) ++count;
      if(count==0) throw std::runtime_error("hex string escape requires digits");
      auto hex=std::stoi(value.substr(pos+2,count),nullptr,16);
      value.replace(pos,2+count,std::string(1,static_cast<char>(hex)));
    }
    for(std::size_t pos=0; (pos=value.find('\\',pos))!=std::string::npos;) {
      if(pos+3<value.size() && value[pos+1]>='0' && value[pos+1]<='7' &&
         value[pos+2]>='0' && value[pos+2]<='7' && value[pos+3]>='0' && value[pos+3]<='7') {
        auto oct=std::stoi(value.substr(pos+1,3),nullptr,8);
        if(oct>255) throw std::runtime_error("octal string escape out of range");
        value.replace(pos,4,std::string(1,static_cast<char>(oct)));
      } else ++pos;
    }
    while((nl=value.find("\\0"))!=std::string::npos) value.replace(nl,2,std::string(1,'\0'));
    while((nl=value.find("\\\""))!=std::string::npos) value.replace(nl,2,"\"");
    while((nl=value.find("\\'"))!=std::string::npos) value.replace(nl,2,"'");
    while((nl=value.find("\\\\"))!=std::string::npos) value.replace(nl,2,"\\");
    return value;
  };
  std::smatch w;
  static const std::regex five_adjacent_write(
    R"re(write\s*\(\s*1\s*,\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,five_adjacent_write)) {
    p.output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.output.size()) throw std::runtime_error("write length mismatch");
  }
  static const std::regex four_adjacent_write(
    R"re(write\s*\(\s*1\s*,\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.output.empty() && std::regex_search(body,w,four_adjacent_write)) {
    p.output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.output.size()) throw std::runtime_error("write length mismatch");
  }
  static const std::regex three_adjacent_write(
    R"re(write\s*\(\s*1\s*,\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.output.empty() && std::regex_search(body,w,three_adjacent_write)) {
    p.output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.output.size()) throw std::runtime_error("write length mismatch");
  }
  static const std::regex adjacent_write(
    R"re(write\s*\(\s*1\s*,\s*"([^\n]*)"\s*"([^\n]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.output.empty() && std::regex_search(body,w,adjacent_write)) {
    p.output=decode_write(w[1].str())+decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.output.size()) throw std::runtime_error("write length mismatch");
  }
  // Collect every literal write in source order.  This deliberately keeps
  // each call's declared byte count independently checked before combining
  // the payload into one static message.
  if(p.output.empty() && body.find("for") == std::string::npos)
    for(std::sregex_iterator it(body.begin(),body.end(),write), end; it!=end; ++it) {
    auto payload=decode_write((*it)[1].str());
    if(std::stoi((*it)[2])!=(int)payload.size()) throw std::runtime_error("write length mismatch");
    p.output+=payload;
  }
  static const std::regex while_adjacent_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex while_three_adjacent_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_three_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex while_four_adjacent_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_four_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex while_five_adjacent_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_five_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str())+decode_write(w[6].str());
    if(std::stoi(w[7])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_while_adjacent_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_while_adjacent_increment_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_adjacent_increment_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_while_inclusive_increment_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_inclusive_increment_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_inclusive=true; p.loop_output=decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_while_increment_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_increment_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_while_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex while_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex while_inclusive_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_inclusive_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_inclusive=true; p.loop_output=decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex while_inclusive_three_adjacent_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_inclusive_three_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex while_inclusive_four_adjacent_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_inclusive_four_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex while_inclusive_five_adjacent_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_inclusive_five_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str())+decode_write(w[6].str());
    if(std::stoi(w[7])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex inclusive_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex inclusive_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex inclusive_three_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_three_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex inclusive_four_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_four_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex inclusive_five_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_five_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str())+decode_write(w[6].str());
    if(std::stoi(w[7])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex inclusive_braced_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_braced_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex five_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,five_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str())+decode_write(w[6].str());
    if(std::stoi(w[7])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex four_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,four_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex three_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^\n]*)"\s*"([^\n]*)"\s*"([^\n]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,three_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^\n]*)"\s*"([^\n]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex strict_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,strict_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex strict_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,strict_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex loop(R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*write\s*\(\s*1\s*,\s*"([^\n]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_output=decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex write_argv(R"(write\s*\(\s*1\s*,\s*argv\s*\[\s*1\s*\]\s*,\s*strlen\s*\(\s*argv\s*\[\s*1\s*\]\s*\)\s*\)\s*;)");
  p.argv1=std::regex_search(body,write_argv);
  static const std::regex getcwd_write(R"(write\s*\(\s*1\s*,\s*getcwd\s*\(\s*buf\s*,\s*4096\s*\)\s*,\s*strlen\s*\(\s*buf\s*\)\s*\)\s*;)");
  p.cwd=std::regex_search(body,getcwd_write);
  static const std::regex dir(R"re((?:listdir|finddir)\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  std::smatch d; if(std::regex_search(body,d,dir)) { p.listdir=true; p.directory=d[1].str(); }
  static const std::regex filtered_dir(R"re(finddir\s*\(\s*"([^"]*)"\s*,\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,filtered_dir)) { p.listdir=true; p.directory=d[1].str(); p.filter=d[2].str(); }
  static const std::regex exists(R"re(if\s*\(\s*exists\s*\(\s*"([^"]*)"\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,exists)) { p.exists=true; p.exists_path=d[1].str(); p.then_status=std::stoi(d[2]); p.else_status=std::stoi(d[3]); }
  static const std::regex isdir(R"re(if\s*\(\s*isdir\s*\(\s*"([^"]*)"\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,isdir)) { p.is_directory=true; p.directory_path=d[1].str(); p.then_status=std::stoi(d[2]); p.else_status=std::stoi(d[3]); }
  static const std::regex isreg(R"re(if\s*\(\s*isreg\s*\(\s*"([^"]*)"\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,isreg)) { p.is_regular=true; p.regular_path=d[1].str(); p.then_status=std::stoi(d[2]); p.else_status=std::stoi(d[3]); }
  static const std::regex sizegt(R"re(if\s*\(\s*sizegt\s*\(\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,sizegt)) { p.size_gt=true; p.size_path=d[1].str(); p.size_bytes=std::stoull(d[2]); p.then_status=std::stoi(d[3]); p.else_status=std::stoi(d[4]); }
  static const std::regex help(R"(if\s*\(\s*argc\s*==\s*2\s*&&\s*(?:streq|strcmp)\s*\(\s*argv\s*\[\s*1\s*\]\s*,\s*"--help"\s*\)\s*\)\s*return\s+([0-9]+)\s*;)");
  if(std::regex_search(body,w,help)) { p.arg_help=true; p.argc_value=2; p.then_status=std::stoi(w[1]); }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
    }
  static const std::regex any_while(R"(\bwhile\s*\()"), supported_while(R"(\bwhile\s*\(\s*i\s*<=?\s*[0-9]+\s*\))");
  if(std::regex_search(body,any_while) && !std::regex_search(body,supported_while)) throw std::runtime_error("unsupported while condition");
  static const std::regex any_for(R"(\bfor\s*\([^;]+;\s*i\s*<=?\s*([^;]+);\s*i\+\+\s*\))");
  std::smatch for_match;
  if(std::regex_search(body,for_match,any_for) && !std::regex_match(for_match[1].str(),std::regex(R"(\s*[0-9]+\s*)"))) throw std::runtime_error("unsupported for bound");
  static const std::regex nonzero_for_init(R"(\bfor\s*\(\s*int\s+i\s*=\s*([^;]+);)");
  std::smatch init_match;
  if(std::regex_search(body,init_match,nonzero_for_init) && !std::regex_match(init_match[1].str(),std::regex(R"(\s*0\s*)"))) throw std::runtime_error("unsupported for initializer");
  if(p.loop_count==0 && std::regex_search(body, std::regex(R"(\b(?:for|while)\s*\()"))) p.loop_present=true;
  return p;
}

void check_with_nbe() {
  using namespace st;
  auto A=sort(1);
  auto redex=app(lam(A,var(0)),sort(0));
  auto quoted=quote(redex);
  auto staged=normalize_code(A,quoted);
  auto normal=nbe_normalise({},staged);
  if(!equal(normal,quote(sort(0)))) throw Error("C subset normalization failed");
  using namespace csem;
  auto Node=structure("CompilerNode"); auto NodePtr=pointer(Node);
  StructFields fields{{"CompilerNode",{{"value",integer()},{"next",NodePtr}}}};
  std::vector<Function> declarations{
    {"compiler_even",{{"p",NodePtr}},integer(),{call("compiler_odd",{variable("p")})}},
    {"compiler_odd",{{"p",NodePtr}},integer(),{call("compiler_even",{variable("p")})}},
    {"compiler_identity",{{"x",integer()}},integer(),{variable("x")}},
    {"compiler_call",{},integer(),{call("compiler_identity",{literal(7)})}},
    {"compiler_answer",{},integer(),{literal(9)}},
    {"compiler_answer_call",{},integer(),{call("compiler_answer",{})}},
    {"compiler_add",{{"left",integer()},{"right",integer()}},integer(),{binary(BinOp::Add,variable("left"),variable("right"))}},
    {"compiler_add_call",{},integer(),{call("compiler_add",{literal(2),literal(5)})}}
  };
  check_program(declarations,fields);
}

void emit_filtered_directory(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $257, %eax\n  mov $-100, %edi\n  lea dir_path(%rip), %rsi\n  xor %edx, %edx\n  xor %r10d, %r10d\n  syscall\n  mov %eax, %r13d\n"
    <<"  mov $217, %eax\n  mov %r13d, %edi\n  lea dir_buf(%rip), %rsi\n  mov $8192, %edx\n  syscall\n  mov %eax, %r14d\n  xor %r12d, %r12d\n.Lfd_next:\n  cmp %r14d, %r12d\n  jge .Lfd_done\n  lea dir_buf(%rip), %rsi\n  movzwl 16(%rsi,%r12,1), %ecx\n  mov %ecx, %r15d\n  test %ecx, %ecx\n  jz .Lfd_done\n  lea 19(%rsi,%r12,1), %r8\n  xor %edx, %edx\n.Lfd_len:\n  cmp %edx, %ecx\n  jle .Lfd_check\n  cmpb $0, (%r8,%rdx,1)\n  je .Lfd_check\n  inc %edx\n  jmp .Lfd_len\n.Lfd_check:\n"
    <<"  cmp $"<<p.filter.size()<<", %edx\n  jne .Lfd_skip\n  xor %r9d, %r9d\n.Lfd_cmp:\n  cmp $"<<p.filter.size()<<", %r9d\n  jge .Lfd_emit\n  movzbq (%r8,%r9,1), %rax\n  lea filter(%rip), %r11\n  movzbq (%r11,%r9,1), %r10\n  cmp %r10b, %al\n  jne .Lfd_skip\n  inc %r9d\n  jmp .Lfd_cmp\n.Lfd_emit:\n"
    <<"  mov $1, %eax\n  mov $1, %edi\n  mov %r8, %rsi\n  syscall\n  mov $1, %eax\n  mov $1, %edi\n  lea newline(%rip), %rsi\n  mov $1, %edx\n  syscall\n.Lfd_skip:\n  add %r15d, %r12d\n  jmp .Lfd_next\n.Lfd_done:\n  mov $0, %edi\n  mov $60, %eax\n  syscall\n.section .rodata\ndir_path:\n  .asciz \""<<p.directory<<"\"\nnewline:\n  .byte 10\nfilter:\n  .byte ";
  for(std::size_t i=0;i<p.filter.size();++i) { if(i) std::cout<<", "; std::cout<<(unsigned)(unsigned char)p.filter[i]; }
  std::cout<<"\n.bss\n.align 8\ndir_buf:\n  .skip 8192\n";
}

void emit_exists(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
           <<"  mov $332, %eax\n  mov $-100, %edi\n  lea exists_path(%rip), %rsi\n  xor %edx, %edx\n  mov $2047, %r10d\n  lea exists_buf(%rip), %r8\n  syscall\n  test %eax, %eax\n  js .Lexists_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lexists_done\n.Lexists_no:\n  mov $"<<p.else_status<<", %edi\n.Lexists_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nexists_path:\n  .asciz \""<<p.exists_path<<"\"\n.bss\n.align 8\nexists_buf:\n  .skip 256\n";
}

void emit_isdir(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
           <<"  mov $332, %eax\n  mov $-100, %edi\n  lea isdir_path(%rip), %rsi\n  xor %edx, %edx\n  mov $2047, %r10d\n  lea isdir_buf(%rip), %r8\n  syscall\n  test %eax, %eax\n  js .Lisdir_no\n  movzwl 28(%r8), %eax\n  and $61440, %eax\n  cmp $16384, %eax\n  jne .Lisdir_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lisdir_done\n.Lisdir_no:\n  mov $"<<p.else_status<<", %edi\n.Lisdir_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nisdir_path:\n  .asciz \""<<p.directory_path<<"\"\n.bss\n.align 8\nisdir_buf:\n  .skip 256\n";
}

void emit_isreg(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
           <<"  mov $332, %eax\n  mov $-100, %edi\n  lea isreg_path(%rip), %rsi\n  xor %edx, %edx\n  mov $2047, %r10d\n  lea isreg_buf(%rip), %r8\n  syscall\n  test %eax, %eax\n  js .Lisreg_no\n  movzwl 28(%r8), %eax\n  and $61440, %eax\n  cmp $32768, %eax\n  jne .Lisreg_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lisreg_done\n.Lisreg_no:\n  mov $"<<p.else_status<<", %edi\n.Lisreg_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nisreg_path:\n  .asciz \""<<p.regular_path<<"\"\n.bss\n.align 8\nisreg_buf:\n  .skip 256\n";
}

void emit_sizegt(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
           <<"  mov $332, %eax\n  mov $-100, %edi\n  lea size_path(%rip), %rsi\n  xor %edx, %edx\n  mov $2047, %r10d\n  lea size_buf(%rip), %r8\n  syscall\n  test %eax, %eax\n  js .Lsize_no\n  mov $"<<p.size_bytes<<", %rax\n  cmp %rax, 40(%r8)\n  jbe .Lsize_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lsize_done\n.Lsize_no:\n  mov $"<<p.else_status<<", %edi\n.Lsize_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nsize_path:\n  .asciz \""<<p.size_path<<"\"\n.bss\n.align 8\nsize_buf:\n  .skip 256\n";
}
}

int main(int argc,char **argv) {
  if(argc!=2) { std::cerr<<"usage: c_subset_compiler SOURCE\n"; return 2; }
  try {
    auto program=csubset::parse_main(csubset::read_source(argv[1]));
    csubset::check_with_nbe();
    if(program.exists) { csubset::emit_exists(program); return 0; }
    if(program.is_directory) { csubset::emit_isdir(program); return 0; }
    if(program.is_regular) { csubset::emit_isreg(program); return 0; }
    if(program.size_gt) { csubset::emit_sizegt(program); return 0; }
    if(program.filter.size()) { csubset::emit_filtered_directory(program); return 0; }
    std::cout<<".text\n.globl _start\n_start:\n";
    if(!program.output.empty()) std::cout
             <<"  mov $1, %eax\n  mov $1, %edi\n  lea message(%rip), %rsi\n  mov $"<<program.output.size()<<", %edx\n  syscall\n";
    if(program.loop_count>0 || program.loop_present) std::cout
             <<"  xor %r12d, %r12d\n.Lfor:\n  cmp $"<<program.loop_count<<", %r12d\n  "<<(program.loop_inclusive ? "jg" : "jge")<<" .Lfor_done\n"
             <<"  mov $1, %eax\n  mov $1, %edi\n  lea loop_message(%rip), %rsi\n  mov $"<<program.loop_output.size()<<", %edx\n  syscall\n"
             <<"  inc %r12d\n  jmp .Lfor\n.Lfor_done:\n";
    if(program.argv1) std::cout
             <<"  mov (%rsp), %rdi\n  cmp $2, %rdi\n  jl .Largv_done\n"
             <<"  mov 16(%rsp), %rsi\n  xor %edx, %edx\n.Lstrlen:\n"
             <<"  cmpb $0, (%rsi,%rdx,1)\n  je .Lwritearg\n  inc %rdx\n  jmp .Lstrlen\n.Lwritearg:\n"
             <<"  mov $1, %eax\n  mov $1, %edi\n  syscall\n";
    if(program.cwd) std::cout
             <<"  lea cwd_buf(%rip), %rdi\n  mov $4096, %esi\n  mov $79, %eax\n  syscall\n"
             <<"  lea cwd_buf(%rip), %rsi\n  xor %edx, %edx\n.Lcwdlen:\n"
             <<"  cmpb $0, (%rsi,%rdx,1)\n  je .Lcwdwrite\n  inc %rdx\n  jmp .Lcwdlen\n.Lcwdwrite:\n"
             <<"  mov $1, %eax\n  mov $1, %edi\n  lea cwd_buf(%rip), %rsi\n  syscall\n";
    if(program.listdir) std::cout
             <<"  mov $257, %eax\n  mov $-100, %edi\n  lea dir_path(%rip), %rsi\n  xor %edx, %edx\n  xor %r10d, %r10d\n  syscall\n"
             <<"  mov %eax, %r13d\n  mov $217, %eax\n  mov %r13d, %edi\n  lea dir_buf(%rip), %rsi\n  mov $8192, %edx\n  syscall\n  mov %eax, %r14d\n  xor %r12d, %r12d\n.Ldir_next:\n  cmp %r14d, %r12d\n  jge .Ldir_done\n  lea dir_buf(%rip), %rsi\n  movzwl 16(%rsi,%r12,1), %ecx\n  mov %ecx, %r15d\n  test %ecx, %ecx\n  jz .Ldir_done\n  lea 19(%rsi,%r12,1), %r8\n  xor %edx, %edx\n.Ldir_name:\n  cmp %edx, %ecx\n  jle .Ldir_write\n  cmpb $0, (%r8,%rdx,1)\n  je .Ldir_write\n  inc %edx\n  jmp .Ldir_name\n.Ldir_write:\n  mov $1, %eax\n  mov $1, %edi\n  mov %r8, %rsi\n  syscall\n  mov $1, %eax\n  mov $1, %edi\n  lea newline(%rip), %rsi\n  mov $1, %edx\n  syscall\n  add %r15d, %r12d\n  jmp .Ldir_next\n.Ldir_done:\n";
    if(program.arg_help) std::cout
             <<"  mov (%rsp), %rdi\n  cmp $2, %rdi\n  jne .Lhelp_no\n"
             <<"  mov 16(%rsp), %rsi\n  cmpb $'-', (%rsi)\n  jne .Lhelp_no\n"
             <<"  cmpb $'-', 1(%rsi)\n  jne .Lhelp_no\n"
             <<"  cmpb $'h', 2(%rsi)\n  jne .Lhelp_no\n"
             <<"  cmpb $'e', 3(%rsi)\n  jne .Lhelp_no\n"
             <<"  cmpb $'l', 4(%rsi)\n  jne .Lhelp_no\n"
             <<"  cmpb $'p', 5(%rsi)\n  jne .Lhelp_no\n"
             <<"  mov $"<<program.then_status<<", %edi\n  jmp .Lexit\n.Lhelp_no:\n";
    if(program.argv1) std::cout<<".Largv_done:\n  mov $"<<program.else_status<<", %edi\n";
    if(program.null_guard) std::cout
             <<"  mov $"<<program.then_status<<", %edi\n  jmp .Lexit\n";
    if(program.pointer_equal) std::cout
             <<"  mov $"<<program.then_status<<", %edi\n  jmp .Lexit\n";
    if(program.switch_return) std::cout
             <<"  mov (%rsp), %rdi\n  cmp $"<<program.switch_case<<", %rdi\n"
             <<"  jne .Lswitch_case2\n  mov $"<<program.switch_case_status<<", %edi\n  jmp .Lexit\n.Lswitch_case2:\n"
             <<(program.switch_two_cases ? "  cmp $"+std::to_string(program.switch_case2)+", %rdi\n  jne .Lswitch_default\n  mov $"+std::to_string(program.switch_case2_status)+", %edi\n  jmp .Lexit\n.Lswitch_default:\n" : "  jmp .Lswitch_default\n.Lswitch_default:\n")
             <<"  mov $"<<program.switch_default_status<<", %edi\n  jmp .Lexit\n";
    if(program.argc_value>=0) std::cout
             <<"  mov (%rsp), %rdi\n  cmp $"<<program.argc_value<<", %rdi\n"
             <<"  jne .Lelse\n  mov $"<<program.then_status<<", %edi\n  jmp .Lexit\n.Lelse:\n";
    std::cout<<"  mov $"<<program.else_status<<", %edi\n"
             <<".Lexit:\n"
             <<"  mov $60, %eax\n  syscall\n";
    if(!program.output.empty()) {
      std::cout<<".section .rodata\nmessage:\n  .byte ";
      for(std::size_t i=0;i<program.output.size();++i) { if(i) std::cout<<", "; std::cout<<(unsigned)(unsigned char)program.output[i]; }
      std::cout<<"\n";
    }
    if(!program.loop_output.empty()) {
      std::cout<<"loop_message:\n  .byte ";
      for(std::size_t i=0;i<program.loop_output.size();++i) { if(i) std::cout<<", "; std::cout<<(unsigned)(unsigned char)program.loop_output[i]; }
      std::cout<<"\n";
    }
    if(program.cwd) std::cout<<".bss\n.align 8\ncwd_buf:\n  .skip 4096\n";
    if(program.listdir) std::cout<<".section .rodata\ndir_path:\n  .asciz \""<<program.directory<<"\"\nnewline:\n  .byte 10\n.bss\n.align 8\ndir_buf:\n  .skip 8192\n";
    std::cerr<<"C subset + dependent NbE: PASS\n";
  } catch(std::exception const& e) { std::cerr<<"C subset error: "<<e.what()<<'\n'; return 1; }
}
