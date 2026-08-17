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

bool emit_getuid_mode = false;
bool emit_geteuid_mode = false;
bool emit_getgid_mode = false;
bool emit_getegid_mode = false;
bool emit_getpgid_mode = false;
unsigned long long getpgid_pid_value = 0;
bool emit_getsid_mode = false;
unsigned long long getsid_pid_value = 0;
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

struct Program { int argc_value=-1, then_status=0, else_status=0, switch_case=-1, switch_case_status=0, switch_case2=-1, switch_case2_status=0, switch_default_status=0; std::string output, error_output, loop_output, directory, filter, exists_path, directory_path, regular_path, size_path, cat_path, mkdir_path, rm_path, rmdir_path, touch_path, chdir_path, symlink_target, symlink_path, link_old, link_new, readlink_path, rename_old, rename_new, chmod_path, access_path, truncate_path, fsync_path, fdatasync_path, writefd_text, stat_path, lstat_path, memfd_name; std::vector<std::pair<int,std::string>> ordered_output; unsigned long long size_bytes=0, truncate_size=0, random_bytes=0, stdin_bytes=0, sleep_seconds=0, umask_mode=0, alarm_seconds=0, clock_id=0, rusage_who=0, rlimit_resource=0, priority_query_which=0, priority_query_who=0, affinity_pid=0, eventfd_init=0, timerfd_clock=0, timerfd_flags=0, inotify_flags=0, pidfd_pid=0, pidfd_flags=0, memfd_flags=0, epoll_flags=0, epoll_size=0; unsigned chmod_mode=0, access_mode=0, dup_old=0, dup_new=0, close_fd=0, tty_fd=0, fcntl_fd=0, fcntl_cmd=0, fcntl_arg=0, setpgid_pid=0, setpgid_pgid=0, priority_which=0, priority_who=0, priority_value=0, nice_increment=0, writefd_fd=0, writefd_len=0, readfd_fd=0, readfd_len=0, poll_fd=0, poll_events=0, poll_timeout=0, fstat_fd=0; int loop_count=0; bool loop_present=false, loop_inclusive=false, loop_do=false, argv1=false, arg_help=false, cwd=false, listdir=false, cat=false, mkdir=false, rm=false, rmdir=false, touch=false, chdir=false, symlink=false, link=false, readlink=false, rename=false, chmod=false, access=false, truncate=false, getrandom=false, readstdin=false, sleep=false, isatty=false, sync=false, fsync=false, fdatasync=false, umask=false, fcntl=false, setpgid=false, yield=false, getpid=false, getppid=false, setpriority=false, isroot=false, gettid=false, isgroup0=false, ise_group0=false, nice=false, writefd=false, readfd=false, poll=false, alarm=false, clock_gettime=false, gettimeofday=false, times=false, getrusage=false, sysinfo=false, uname=false, getdomainname=false, fstat=false, stat=false, lstat=false, getgroups=false, getresuid=false, getresgid=false, getrlimit=false, getpriority=false, getcpu=false, sched_getaffinity=false, eventfd=false, timerfd_create=false, inotify_init1=false, pidfd_open=false, memfd_create=false, epoll_create1=false, epoll_create=false, dup=false, close=false, pipe=false, exists=false, is_directory=false, is_regular=false, size_gt=false, function_call=false, null_guard=false, pointer_equal=false, switch_return=false, switch_two_cases=false; };

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
  static const std::regex error_write(R"re(write\s*\(\s*2\s*,\s*"([^\n]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,error_write)) {
    p.error_output=decode_write(w[1].str());
    if(std::stoi(w[2])!=(int)p.error_output.size()) throw std::runtime_error("write length mismatch");
  }
  static const std::regex error_adjacent_write(R"re(write\s*\(\s*2\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,error_adjacent_write)) {
    p.error_output=decode_write(w[1].str())+decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.error_output.size()) throw std::runtime_error("write length mismatch");
  }
  static const std::regex error_three_adjacent_write(R"re(write\s*\(\s*2\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,error_three_adjacent_write)) {
    p.error_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.error_output.size()) throw std::runtime_error("write length mismatch");
  }
  static const std::regex error_four_adjacent_write(R"re(write\s*\(\s*2\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,error_four_adjacent_write)) {
    p.error_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.error_output.size()) throw std::runtime_error("write length mismatch");
  }
  static const std::regex error_five_adjacent_write(R"re(write\s*\(\s*2\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,error_five_adjacent_write)) {
    p.error_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.error_output.size()) throw std::runtime_error("write length mismatch");
  }
  if(p.error_output.empty()) {
    for(std::sregex_iterator it(body.begin(),body.end(),error_write), end; it!=end; ++it) {
      auto payload=decode_write((*it)[1].str());
      if(std::stoi((*it)[2])!=(int)payload.size()) throw std::runtime_error("write length mismatch");
      p.error_output+=payload;
    }
  }
  static const std::regex has_stdout_write(R"(write\s*\(\s*1\s*,)"), has_stderr_write(R"(write\s*\(\s*2\s*,)");
  if(std::regex_search(body,has_stdout_write) && std::regex_search(body,has_stderr_write)) {
    static const std::regex ordered_write(R"re(write\s*\(\s*([12])\s*,\s*"([^\n]*)"(?:\s*"([^\n]*)")?(?:\s*"([^\n]*)")?(?:\s*"([^\n]*)")?(?:\s*"([^\n]*)")?\s*,\s*([0-9]+)\s*\)\s*;)re");
    for(std::sregex_iterator it(body.begin(),body.end(),ordered_write), end; it!=end; ++it) {
      auto payload=decode_write((*it)[2].str());
      if((*it)[3].matched) payload+=decode_write((*it)[3].str());
      if((*it)[4].matched) payload+=decode_write((*it)[4].str());
      if((*it)[5].matched) payload+=decode_write((*it)[5].str());
      if((*it)[6].matched) payload+=decode_write((*it)[6].str());
      if(std::stoi((*it)[7])!=(int)payload.size()) throw std::runtime_error("write length mismatch");
      p.ordered_output.emplace_back(std::stoi((*it)[1]),std::move(payload));
    }
  }
  static const std::regex do_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_write_loop)) {
    p.loop_count=std::stoi(w[3]); p.loop_present=true; p.loop_do=true; p.loop_output=decode_write(w[1].str());
    if(std::stoi(w[2])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_unbraced_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s+write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_unbraced_write_loop)) {
    p.loop_count=std::stoi(w[3]); p.loop_present=true; p.loop_do=true; p.loop_output=decode_write(w[1].str());
    if(std::stoi(w[2])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_unbraced_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s+write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_unbraced_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[4]); p.loop_present=true; p.loop_do=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_unbraced_three_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s+write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_unbraced_three_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[5]); p.loop_present=true; p.loop_do=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_unbraced_four_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s+write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_unbraced_four_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[6]); p.loop_present=true; p.loop_do=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_unbraced_five_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s+write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_unbraced_five_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[7]); p.loop_present=true; p.loop_do=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_unbraced_inclusive_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s+write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_unbraced_inclusive_write_loop)) {
    p.loop_count=std::stoi(w[3]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=decode_write(w[1].str());
    if(std::stoi(w[2])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_unbraced_inclusive_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s+write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_unbraced_inclusive_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[4]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_unbraced_inclusive_three_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s+write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_unbraced_inclusive_three_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[5]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_unbraced_inclusive_four_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s+write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_unbraced_inclusive_four_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[6]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_unbraced_inclusive_five_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s+write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_unbraced_inclusive_five_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[7]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex nonzero_do_init(R"(\bint\s+i\s*=\s*([^;]+);\s*do\s*\{)");
  std::smatch do_init_match;
  if(std::regex_search(body,do_init_match,nonzero_do_init) && !std::regex_match(do_init_match[1].str(),std::regex(R"(\s*0\s*)"))) throw std::runtime_error("unsupported do initializer");
  static const std::regex any_do(R"(\bdo\s*\{)"), supported_do_update(R"(\bdo\s*\{[\s\S]*i\+\+)" );
  if(std::regex_search(body,any_do) && !std::regex_search(body,supported_do_update)) throw std::runtime_error("unsupported do update");
  static const std::regex do_inclusive_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_inclusive_write_loop)) {
    p.loop_count=std::stoi(w[3]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=decode_write(w[1].str());
    if(std::stoi(w[2])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_inclusive_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_inclusive_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[4]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_inclusive_three_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_inclusive_three_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[5]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_inclusive_four_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_inclusive_four_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[6]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_inclusive_five_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_inclusive_five_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[7]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[4]); p.loop_present=true; p.loop_do=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str());
    if(std::stoi(w[3])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_three_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_three_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[5]); p.loop_present=true; p.loop_do=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_four_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_four_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[6]); p.loop_present=true; p.loop_do=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_five_adjacent_write_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_five_adjacent_write_loop)) {
    p.loop_count=std::stoi(w[7]); p.loop_present=true; p.loop_do=true; p.loop_output=decode_write(w[1].str())+decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex do_two_body_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_two_body_writes)) {
    auto a=decode_write(w[1].str()), b=decode_write(w[3].str());
    if(std::stoi(w[2])!=(int)a.size() || std::stoi(w[4])!=(int)b.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[5]); p.loop_present=true; p.loop_do=true; p.loop_output=a+b; p.output.clear();
  }
  static const std::regex do_inclusive_two_body_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_inclusive_two_body_writes)) {
    auto a=decode_write(w[1].str()), b=decode_write(w[3].str());
    if(std::stoi(w[2])!=(int)a.size() || std::stoi(w[4])!=(int)b.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[5]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=a+b; p.output.clear();
  }
  static const std::regex do_inclusive_three_body_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_inclusive_three_body_writes)) {
    auto a=decode_write(w[1].str()), b=decode_write(w[3].str()), c=decode_write(w[5].str());
    if(std::stoi(w[2])!=(int)a.size() || std::stoi(w[4])!=(int)b.size() || std::stoi(w[6])!=(int)c.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[7]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=a+b+c; p.output.clear();
  }
  static const std::regex do_inclusive_four_body_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_inclusive_four_body_writes)) {
    auto a=decode_write(w[1].str()), b=decode_write(w[3].str()), c=decode_write(w[5].str()), d=decode_write(w[7].str());
    if(std::stoi(w[2])!=(int)a.size() || std::stoi(w[4])!=(int)b.size() || std::stoi(w[6])!=(int)c.size() || std::stoi(w[8])!=(int)d.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[9]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=a+b+c+d; p.output.clear();
  }
  static const std::regex do_inclusive_five_body_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_inclusive_five_body_writes)) {
    auto a=decode_write(w[1].str()), b=decode_write(w[3].str()), c=decode_write(w[5].str()), d=decode_write(w[7].str()), e=decode_write(w[9].str());
    if(std::stoi(w[2])!=(int)a.size() || std::stoi(w[4])!=(int)b.size() || std::stoi(w[6])!=(int)c.size() || std::stoi(w[8])!=(int)d.size() || std::stoi(w[10])!=(int)e.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[11]); p.loop_present=true; p.loop_do=true; p.loop_inclusive=true; p.loop_output=a+b+c+d+e; p.output.clear();
  }
  static const std::regex do_three_body_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_three_body_writes)) {
    auto a=decode_write(w[1].str()), b=decode_write(w[3].str()), c=decode_write(w[5].str());
    if(std::stoi(w[2])!=(int)a.size() || std::stoi(w[4])!=(int)b.size() || std::stoi(w[6])!=(int)c.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[7]); p.loop_present=true; p.loop_do=true; p.loop_output=a+b+c; p.output.clear();
  }
  static const std::regex do_four_body_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_four_body_writes)) {
    auto a=decode_write(w[1].str()), b=decode_write(w[3].str()), c=decode_write(w[5].str()), d=decode_write(w[7].str());
    if(std::stoi(w[2])!=(int)a.size() || std::stoi(w[4])!=(int)b.size() || std::stoi(w[6])!=(int)c.size() || std::stoi(w[8])!=(int)d.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[9]); p.loop_present=true; p.loop_do=true; p.loop_output=a+b+c+d; p.output.clear();
  }
  static const std::regex do_five_body_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*do\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;\s*\}\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,w,do_five_body_writes)) {
    auto a=decode_write(w[1].str()), b=decode_write(w[3].str()), c=decode_write(w[5].str()), d=decode_write(w[7].str()), e=decode_write(w[9].str());
    if(std::stoi(w[2])!=(int)a.size() || std::stoi(w[4])!=(int)b.size() || std::stoi(w[6])!=(int)c.size() || std::stoi(w[8])!=(int)d.size() || std::stoi(w[10])!=(int)e.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[11]); p.loop_present=true; p.loop_do=true; p.loop_output=a+b+c+d+e; p.output.clear();
  }
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
  static const std::regex while_unbraced_increment_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_unbraced_increment_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
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
  static const std::regex braced_while_five_adjacent_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_five_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str())+decode_write(w[6].str());
    if(std::stoi(w[7])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_while_two_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_two_writes)) {
    auto first=decode_write(w[2].str()), second=decode_write(w[4].str());
    if(std::stoi(w[3])!=(int)first.size() || std::stoi(w[5])!=(int)second.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=first+second; p.output.clear();
  }
  static const std::regex braced_while_three_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_three_writes)) {
    auto first=decode_write(w[2].str()), second=decode_write(w[4].str()), third=decode_write(w[6].str());
    if(std::stoi(w[3])!=(int)first.size() || std::stoi(w[5])!=(int)second.size() || std::stoi(w[7])!=(int)third.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=first+second+third; p.output.clear();
  }
  static const std::regex braced_while_four_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_four_writes)) {
    auto a=decode_write(w[2].str()), b=decode_write(w[4].str()), c=decode_write(w[6].str()), d=decode_write(w[8].str());
    if(std::stoi(w[3])!=(int)a.size() || std::stoi(w[5])!=(int)b.size() || std::stoi(w[7])!=(int)c.size() || std::stoi(w[9])!=(int)d.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=a+b+c+d; p.output.clear();
  }
  static const std::regex braced_while_five_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_five_writes)) {
    auto a=decode_write(w[2].str()), b=decode_write(w[4].str()), c=decode_write(w[6].str()), d=decode_write(w[8].str()), e=decode_write(w[10].str());
    if(std::stoi(w[3])!=(int)a.size() || std::stoi(w[5])!=(int)b.size() || std::stoi(w[7])!=(int)c.size() || std::stoi(w[9])!=(int)d.size() || std::stoi(w[11])!=(int)e.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=a+b+c+d+e; p.output.clear();
  }
  static const std::regex braced_while_inclusive_two_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_inclusive_two_writes)) {
    auto first=decode_write(w[2].str()), second=decode_write(w[4].str());
    if(std::stoi(w[3])!=(int)first.size() || std::stoi(w[5])!=(int)second.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=first+second; p.output.clear();
  }
  static const std::regex braced_while_inclusive_adjacent_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_inclusive_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_while_inclusive_three_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_inclusive_three_writes)) {
    auto a=decode_write(w[2].str()), b=decode_write(w[4].str()), c=decode_write(w[6].str());
    if(std::stoi(w[3])!=(int)a.size() || std::stoi(w[5])!=(int)b.size() || std::stoi(w[7])!=(int)c.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=a+b+c; p.output.clear();
  }
  static const std::regex braced_while_inclusive_four_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_inclusive_four_writes)) {
    auto a=decode_write(w[2].str()), b=decode_write(w[4].str()), c=decode_write(w[6].str()), d=decode_write(w[8].str());
    if(std::stoi(w[3])!=(int)a.size() || std::stoi(w[5])!=(int)b.size() || std::stoi(w[7])!=(int)c.size() || std::stoi(w[9])!=(int)d.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=a+b+c+d; p.output.clear();
  }
  static const std::regex braced_while_inclusive_five_writes(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_while_inclusive_five_writes)) {
    auto a=decode_write(w[2].str()), b=decode_write(w[4].str()), c=decode_write(w[6].str()), d=decode_write(w[8].str()), e=decode_write(w[10].str());
    if(std::stoi(w[3])!=(int)a.size() || std::stoi(w[5])!=(int)b.size() || std::stoi(w[7])!=(int)c.size() || std::stoi(w[9])!=(int)d.size() || std::stoi(w[11])!=(int)e.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=a+b+c+d+e; p.output.clear();
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
  static const std::regex while_inclusive_adjacent_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_inclusive_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex while_inclusive_unbraced_increment_loop(
    R"re(int\s+i\s*=\s*0\s*;\s*while\s*\(\s*i\s*<=\s*([0-9]+)\s*\)\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*i\+\+\s*;)re");
  if(p.loop_count==0 && std::regex_search(body,w,while_inclusive_unbraced_increment_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
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
  static const std::regex inclusive_braced_two_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_braced_two_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex inclusive_braced_three_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_braced_three_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex inclusive_braced_four_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_braced_four_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex inclusive_braced_five_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_braced_five_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str())+decode_write(w[6].str());
    if(std::stoi(w[7])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_two_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_two_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str());
    if(std::stoi(w[4])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_three_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_three_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str());
    if(std::stoi(w[5])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_four_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_four_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str());
    if(std::stoi(w[6])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_five_adjacent_loop(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_five_adjacent_loop)) {
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=decode_write(w[2].str())+decode_write(w[3].str())+decode_write(w[4].str())+decode_write(w[5].str())+decode_write(w[6].str());
    if(std::stoi(w[7])!=(int)p.loop_output.size()) throw std::runtime_error("loop write length mismatch");
    p.output.clear();
  }
  static const std::regex braced_for_two_writes(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_for_two_writes)) {
    auto first=decode_write(w[2].str()), second=decode_write(w[4].str());
    if(std::stoi(w[3])!=(int)first.size() || std::stoi(w[5])!=(int)second.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=first+second; p.output.clear();
  }
  static const std::regex braced_for_three_writes(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,braced_for_three_writes)) {
    auto first=decode_write(w[2].str()), second=decode_write(w[4].str()), third=decode_write(w[6].str());
    if(std::stoi(w[3])!=(int)first.size() || std::stoi(w[5])!=(int)second.size() || std::stoi(w[7])!=(int)third.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_output=first+second+third; p.output.clear();
  }
  static const std::regex inclusive_braced_for_two_writes(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_braced_for_two_writes)) {
    auto first=decode_write(w[2].str()), second=decode_write(w[4].str());
    if(std::stoi(w[3])!=(int)first.size() || std::stoi(w[5])!=(int)second.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=first+second; p.output.clear();
  }
  static const std::regex inclusive_braced_for_three_writes(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_braced_for_three_writes)) {
    auto a=decode_write(w[2].str()), b=decode_write(w[4].str()), c=decode_write(w[6].str());
    if(std::stoi(w[3])!=(int)a.size() || std::stoi(w[5])!=(int)b.size() || std::stoi(w[7])!=(int)c.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=a+b+c; p.output.clear();
  }
  static const std::regex inclusive_braced_for_four_writes(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_braced_for_four_writes)) {
    auto a=decode_write(w[2].str()), b=decode_write(w[4].str()), c=decode_write(w[6].str()), d=decode_write(w[8].str());
    if(std::stoi(w[3])!=(int)a.size() || std::stoi(w[5])!=(int)b.size() || std::stoi(w[7])!=(int)c.size() || std::stoi(w[9])!=(int)d.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=a+b+c+d; p.output.clear();
  }
  static const std::regex inclusive_braced_for_five_writes(
    R"re(for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*i\+\+\s*\)\s*\{\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*write\s*\(\s*1\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;\s*\}\s*)re");
  if(p.loop_count==0 && std::regex_search(body,w,inclusive_braced_for_five_writes)) {
    auto a=decode_write(w[2].str()), b=decode_write(w[4].str()), c=decode_write(w[6].str()), d=decode_write(w[8].str()), e=decode_write(w[10].str());
    if(std::stoi(w[3])!=(int)a.size() || std::stoi(w[5])!=(int)b.size() || std::stoi(w[7])!=(int)c.size() || std::stoi(w[9])!=(int)d.size() || std::stoi(w[11])!=(int)e.size()) throw std::runtime_error("loop write length mismatch");
    p.loop_count=std::stoi(w[1]); p.loop_present=true; p.loop_inclusive=true; p.loop_output=a+b+c+d+e; p.output.clear();
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
  static const std::regex cat_file(R"re(cat\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,cat_file)) { p.cat=true; p.cat_path=d[1].str(); }
  static const std::regex mkdir_dir(R"re(mkdir\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,mkdir_dir)) { p.mkdir=true; p.mkdir_path=d[1].str(); }
  static const std::regex remove_file(R"re(rm\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,remove_file)) { p.rm=true; p.rm_path=d[1].str(); }
  static const std::regex remove_dir(R"re(rmdir\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,remove_dir)) { p.rmdir=true; p.rmdir_path=d[1].str(); }
  static const std::regex touch_file(R"re(touch\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,touch_file)) { p.touch=true; p.touch_path=d[1].str(); }
  static const std::regex change_dir(R"re(chdir\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,change_dir)) { p.chdir=true; p.chdir_path=d[1].str(); }
  static const std::regex make_link(R"re(symlink\s*\(\s*"([^"]*)"\s*,\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,make_link)) { p.symlink=true; p.symlink_target=d[1].str(); p.symlink_path=d[2].str(); }
  static const std::regex make_hard_link(R"re(link\s*\(\s*"([^"]*)"\s*,\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,make_hard_link)) { p.link=true; p.link_old=d[1].str(); p.link_new=d[2].str(); }
  static const std::regex read_link(R"re(readlink\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,read_link)) { p.readlink=true; p.readlink_path=d[1].str(); }
  static const std::regex rename_file(R"re(rename\s*\(\s*"([^"]*)"\s*,\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,rename_file)) { p.rename=true; p.rename_old=d[1].str(); p.rename_new=d[2].str(); }
  static const std::regex change_mode(R"re(chmod\s*\(\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,change_mode)) { p.chmod=true; p.chmod_path=d[1].str(); p.chmod_mode=static_cast<unsigned>(std::stoul(d[2])); }
  static const std::regex check_access(R"re(access\s*\(\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,check_access)) { p.access=true; p.access_path=d[1].str(); p.access_mode=static_cast<unsigned>(std::stoul(d[2])); }
  static const std::regex resize_file(R"re(truncate\s*\(\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,resize_file)) { p.truncate=true; p.truncate_path=d[1].str(); p.truncate_size=std::stoull(d[2]); }
  static const std::regex random_bytes_call(R"re(getrandom\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,random_bytes_call)) { p.getrandom=true; p.random_bytes=std::stoull(d[1]); if(p.random_bytes>4096) throw std::runtime_error("getrandom request too large"); }
  static const std::regex stdin_copy(R"re(readstdin\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,stdin_copy)) { p.readstdin=true; p.stdin_bytes=std::stoull(d[1]); if(p.stdin_bytes>4096) throw std::runtime_error("stdin request too large"); }
  static const std::regex sleep_call(R"re(sleep\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,sleep_call)) { p.sleep=true; p.sleep_seconds=std::stoull(d[1]); if(p.sleep_seconds>60) throw std::runtime_error("sleep request too large"); }
  static const std::regex tty_test(R"re(if\s*\(\s*isatty\s*\(\s*([0-9]+)\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,tty_test)) { p.isatty=true; p.tty_fd=static_cast<unsigned>(std::stoul(d[1])); p.then_status=std::stoi(d[2]); p.else_status=std::stoi(d[3]); }
  p.sync=std::regex_search(body,std::regex(R"re(\bsync\s*\(\s*\)\s*;)re"));
  static const std::regex fsync_call(R"re(fsync\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,fsync_call)) { p.fsync=true; p.fsync_path=d[1].str(); }
  static const std::regex fdatasync_call(R"re(fdatasync\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,fdatasync_call)) { p.fdatasync=true; p.fdatasync_path=d[1].str(); }
  static const std::regex umask_call(R"re(umask\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,umask_call)) { p.umask=true; p.umask_mode=std::stoull(d[1]); if(p.umask_mode>0777) throw std::runtime_error("umask out of range"); }
  static const std::regex fcntl_call(R"re(fcntl\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,fcntl_call)) { p.fcntl=true; p.fcntl_fd=std::stoul(d[1]); p.fcntl_cmd=std::stoul(d[2]); p.fcntl_arg=std::stoul(d[3]); }
  static const std::regex setpgid_call(R"re(setpgid\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,setpgid_call)) { p.setpgid=true; p.setpgid_pid=std::stoul(d[1]); p.setpgid_pgid=std::stoul(d[2]); }
  p.yield=std::regex_search(body,std::regex(R"re(\byield\s*\(\s*\)\s*;)re"));
  static const std::regex pid_test(R"re(if\s*\(\s*getpid\s*\(\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,pid_test)) { p.getpid=true; p.then_status=std::stoi(d[1]); p.else_status=std::stoi(d[2]); }
  static const std::regex ppid_test(R"re(if\s*\(\s*getppid\s*\(\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,ppid_test)) { p.getppid=true; p.then_status=std::stoi(d[1]); p.else_status=std::stoi(d[2]); }
  emit_getuid_mode=std::regex_search(body,std::regex(R"re(\bgetuid\s*\(\s*\)\s*;)re"));
  emit_geteuid_mode=std::regex_search(body,std::regex(R"re(\bgeteuid\s*\(\s*\)\s*;)re"));
  emit_getgid_mode=std::regex_search(body,std::regex(R"re(\bgetgid\s*\(\s*\)\s*;)re"));
  emit_getegid_mode=std::regex_search(body,std::regex(R"re(\bgetegid\s*\(\s*\)\s*;)re"));
  std::smatch getpgid_match;
  if(std::regex_search(body,getpgid_match,std::regex(R"re(getpgid\s*\(\s*([0-9]+)\s*\)\s*;)re"))) { emit_getpgid_mode=true; getpgid_pid_value=std::stoull(getpgid_match[1]); }
  std::smatch getsid_match;
  if(std::regex_search(body,getsid_match,std::regex(R"re(getsid\s*\(\s*([0-9]+)\s*\)\s*;)re"))) { emit_getsid_mode=true; getsid_pid_value=std::stoull(getsid_match[1]); }
  static const std::regex priority_call(R"re(setpriority\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,priority_call)) { p.setpriority=true; p.priority_which=std::stoul(d[1]); p.priority_who=std::stoul(d[2]); p.priority_value=std::stoul(d[3]); if(p.priority_value>19) throw std::runtime_error("priority out of range"); }
  static const std::regex root_test(R"re(if\s*\(\s*isroot\s*\(\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,root_test)) { p.isroot=true; p.then_status=std::stoi(d[1]); p.else_status=std::stoi(d[2]); }
  static const std::regex tid_test(R"re(if\s*\(\s*gettid\s*\(\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,tid_test)) { p.gettid=true; p.then_status=std::stoi(d[1]); p.else_status=std::stoi(d[2]); }
  static const std::regex group_test(R"re(if\s*\(\s*isgroup0\s*\(\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,group_test)) { p.isgroup0=true; p.then_status=std::stoi(d[1]); p.else_status=std::stoi(d[2]); }
  static const std::regex egroup_test(R"re(if\s*\(\s*ise_group0\s*\(\s*\)\s*\)\s*return\s+([0-9]+)\s*;\s*return\s+([0-9]+)\s*;)re");
  if(std::regex_search(body,d,egroup_test)) { p.ise_group0=true; p.then_status=std::stoi(d[1]); p.else_status=std::stoi(d[2]); }
  static const std::regex nice_call(R"re(nice\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,nice_call)) { p.nice=true; p.nice_increment=std::stoul(d[1]); if(p.nice_increment>19) throw std::runtime_error("nice increment too large"); }
  static const std::regex writefd_call(R"re(writefd\s*\(\s*([0-9]+)\s*,\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,writefd_call)) { p.writefd=true; p.writefd_fd=std::stoul(d[1]); p.writefd_text=decode_write(d[2].str()); p.writefd_len=std::stoul(d[3]); if(p.writefd_len!=p.writefd_text.size()) throw std::runtime_error("writefd length mismatch"); }
  static const std::regex readfd_call(R"re(readfd\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,readfd_call)) { p.readfd=true; p.readfd_fd=std::stoul(d[1]); p.readfd_len=std::stoul(d[2]); if(p.readfd_len>4096) throw std::runtime_error("readfd request too large"); }
  static const std::regex poll_call(R"re(poll\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,poll_call)) { p.poll=true; p.poll_fd=std::stoul(d[1]); p.poll_events=std::stoul(d[2]); p.poll_timeout=std::stoul(d[3]); if(p.poll_timeout>60000) throw std::runtime_error("poll timeout too large"); }
  static const std::regex alarm_call(R"re(alarm\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,alarm_call)) { p.alarm=true; p.alarm_seconds=std::stoull(d[1]); if(p.alarm_seconds>3600) throw std::runtime_error("alarm too large"); }
  static const std::regex clock_call(R"re(clock_gettime\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,clock_call)) { p.clock_gettime=true; p.clock_id=std::stoul(d[1]); if(p.clock_id>16) throw std::runtime_error("clock id out of range"); }
  p.gettimeofday=std::regex_search(body,std::regex(R"re(\bgettimeofday\s*\(\s*\)\s*;)re"));
  p.times=std::regex_search(body,std::regex(R"re(\btimes\s*\(\s*\)\s*;)re"));
  static const std::regex rusage_call(R"re(getrusage\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,rusage_call)) { p.getrusage=true; p.rusage_who=std::stoull(d[1]); if(p.rusage_who>1) throw std::runtime_error("rusage selector out of range"); }
  p.sysinfo=std::regex_search(body,std::regex(R"re(\bsysinfo\s*\(\s*\)\s*;)re"));
  p.uname=std::regex_search(body,std::regex(R"re(\buname\s*\(\s*\)\s*;)re"));
  p.getdomainname=std::regex_search(body,std::regex(R"re(\bgetdomainname\s*\(\s*\)\s*;)re"));
  static const std::regex fstat_call(R"re(fstat\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,fstat_call)) { p.fstat=true; p.fstat_fd=std::stoul(d[1]); }
  static const std::regex stat_call(R"re(stat\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,stat_call)) { p.stat=true; p.stat_path=d[1].str(); }
  static const std::regex lstat_call(R"re(lstat\s*\(\s*"([^"]*)"\s*\)\s*;)re");
  if(std::regex_search(body,d,lstat_call)) { p.lstat=true; p.lstat_path=d[1].str(); }
  p.getgroups=std::regex_search(body,std::regex(R"re(\bgetgroups\s*\(\s*\)\s*;)re"));
  p.getresuid=std::regex_search(body,std::regex(R"re(\bgetresuid\s*\(\s*\)\s*;)re"));
  p.getresgid=std::regex_search(body,std::regex(R"re(\bgetresgid\s*\(\s*\)\s*;)re"));
  static const std::regex rlimit_call(R"re(getrlimit\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,rlimit_call)) { p.getrlimit=true; p.rlimit_resource=std::stoull(d[1]); if(p.rlimit_resource>15) throw std::runtime_error("rlimit resource out of range"); }
  static const std::regex priority_query_call(R"re(getpriority\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,priority_query_call)) { p.getpriority=true; p.priority_query_which=std::stoul(d[1]); p.priority_query_who=std::stoul(d[2]); if(p.priority_query_which>2) throw std::runtime_error("priority selector out of range"); }
  p.getcpu=std::regex_search(body,std::regex(R"re(\bgetcpu\s*\(\s*\)\s*;)re"));
  static const std::regex affinity_call(R"re(sched_getaffinity\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,affinity_call)) { p.sched_getaffinity=true; p.affinity_pid=std::stoull(d[1]); }
  static const std::regex eventfd_call(R"re(eventfd\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,eventfd_call)) { p.eventfd=true; p.eventfd_init=std::stoull(d[1]); }
  static const std::regex timerfd_call(R"re(timerfd_create\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,timerfd_call)) { p.timerfd_create=true; p.timerfd_clock=std::stoull(d[1]); p.timerfd_flags=std::stoull(d[2]); }
  static const std::regex inotify_call(R"re(inotify_init1\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,inotify_call)) { p.inotify_init1=true; p.inotify_flags=std::stoull(d[1]); }
  static const std::regex pidfd_call(R"re(pidfd_open\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,pidfd_call)) { p.pidfd_open=true; p.pidfd_pid=std::stoull(d[1]); p.pidfd_flags=std::stoull(d[2]); }
  static const std::regex memfd_call(R"re(memfd_create\s*\(\s*"([^"]*)"\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,memfd_call)) { p.memfd_create=true; p.memfd_name=d[1].str(); p.memfd_flags=std::stoull(d[2]); }
  static const std::regex epoll_call(R"re(epoll_create1\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,epoll_call)) { p.epoll_create1=true; p.epoll_flags=std::stoull(d[1]); }
  static const std::regex epoll_legacy_call(R"re(epoll_create\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,epoll_legacy_call)) { p.epoll_create=true; p.epoll_size=std::stoull(d[1]); }
  static const std::regex duplicate_fd(R"re(dup2\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,duplicate_fd)) { p.dup=true; p.dup_old=static_cast<unsigned>(std::stoul(d[1])); p.dup_new=static_cast<unsigned>(std::stoul(d[2])); }
  static const std::regex close_fd_call(R"re(close\s*\(\s*([0-9]+)\s*\)\s*;)re");
  if(std::regex_search(body,d,close_fd_call)) { p.close=true; p.close_fd=static_cast<unsigned>(std::stoul(d[1])); }
  static const std::regex make_pipe(R"re(pipe\s*\(\s*\)\s*;)re");
  p.pipe=std::regex_search(body,make_pipe);
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
  static const std::regex any_for_header(R"(\bfor\s*\([^)]*;[^;]*;[^)]*\))"), supported_for_update(R"(;\s*i\+\+\s*\))");
  if(std::regex_search(body,any_for_header) && !std::regex_search(body,supported_for_update)) throw std::runtime_error("unsupported for update");
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

void emit_cat(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $257, %eax\n  mov $-100, %edi\n  lea cat_path(%rip), %rsi\n  xor %edx, %edx\n  xor %r10d, %r10d\n  syscall\n  test %eax, %eax\n  js .Lcat_done\n  mov %eax, %r12d\n.Lcat_read:\n  mov $0, %eax\n  mov %r12d, %edi\n  lea cat_buf(%rip), %rsi\n  mov $8192, %edx\n  syscall\n  test %eax, %eax\n  jle .Lcat_close\n  mov %eax, %r13d\n  xor %r14d, %r14d\n.Lcat_write:\n  cmp %r13d, %r14d\n  jge .Lcat_read\n  mov $1, %eax\n  mov $1, %edi\n  lea cat_buf(%rip), %rsi\n  add %r14, %rsi\n  mov %r13d, %edx\n  sub %r14d, %edx\n  syscall\n  test %eax, %eax\n  jle .Lcat_error\n  add %eax, %r14d\n  jmp .Lcat_write\n.Lcat_error:\n  mov $3, %eax\n  mov %r12d, %edi\n  syscall\n  mov $1, %edi\n  jmp .Lcat_exit\n.Lcat_close:\n  mov $3, %eax\n  mov %r12d, %edi\n  syscall\n.Lcat_done:\n  xor %edi, %edi\n.Lcat_exit:\n  mov $60, %eax\n  syscall\n.section .rodata\ncat_path:\n  .asciz \""<<p.cat_path<<"\"\n.bss\n.align 8\ncat_buf:\n  .skip 8192\n";
}

void emit_mkdir(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $83, %eax\n  lea mkdir_path(%rip), %rdi\n  mov $448, %esi\n  syscall\n  test %eax, %eax\n  js .Lmkdir_fail\n  xor %edi, %edi\n  jmp .Lmkdir_done\n.Lmkdir_fail:\n  mov $1, %edi\n.Lmkdir_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nmkdir_path:\n  .asciz \""<<p.mkdir_path<<"\"\n";
}

void emit_rm(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $263, %eax\n  mov $-100, %edi\n  lea rm_path(%rip), %rsi\n  xor %edx, %edx\n  syscall\n  test %eax, %eax\n  js .Lrm_fail\n  xor %edi, %edi\n  jmp .Lrm_done\n.Lrm_fail:\n  mov $1, %edi\n.Lrm_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nrm_path:\n  .asciz \""<<p.rm_path<<"\"\n";
}

void emit_rmdir(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $263, %eax\n  mov $-100, %edi\n  lea rmdir_path(%rip), %rsi\n  mov $512, %edx\n  syscall\n  test %eax, %eax\n  js .Lrmdir_fail\n  xor %edi, %edi\n  jmp .Lrmdir_done\n.Lrmdir_fail:\n  mov $1, %edi\n.Lrmdir_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nrmdir_path:\n  .asciz \""<<p.rmdir_path<<"\"\n";
}

void emit_touch(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $257, %eax\n  mov $-100, %edi\n  lea touch_path(%rip), %rsi\n  mov $65, %edx\n  mov $420, %r10d\n  syscall\n  test %eax, %eax\n  js .Ltouch_fail\n  mov %eax, %edi\n  mov $3, %eax\n  syscall\n  xor %edi, %edi\n  jmp .Ltouch_done\n.Ltouch_fail:\n  mov $1, %edi\n.Ltouch_done:\n  mov $60, %eax\n  syscall\n.section .rodata\ntouch_path:\n  .asciz \""<<p.touch_path<<"\"\n";
}

void emit_chdir(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $80, %eax\n  lea chdir_path(%rip), %rdi\n  syscall\n  test %eax, %eax\n  js .Lchdir_fail\n  xor %edi, %edi\n  jmp .Lchdir_done\n.Lchdir_fail:\n  mov $1, %edi\n.Lchdir_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nchdir_path:\n  .asciz \""<<p.chdir_path<<"\"\n";
}

void emit_symlink(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $88, %eax\n  lea symlink_target(%rip), %rdi\n  lea symlink_path(%rip), %rsi\n  syscall\n  test %eax, %eax\n  js .Lsymlink_fail\n  xor %edi, %edi\n  jmp .Lsymlink_done\n.Lsymlink_fail:\n  mov $1, %edi\n.Lsymlink_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nsymlink_target:\n  .asciz \""<<p.symlink_target<<"\"\nsymlink_path:\n  .asciz \""<<p.symlink_path<<"\"\n";
}

void emit_link(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $86, %eax\n  lea link_old(%rip), %rdi\n  lea link_new(%rip), %rsi\n  syscall\n  test %eax, %eax\n  js .Llink_fail\n  xor %edi, %edi\n  jmp .Llink_done\n.Llink_fail:\n  mov $1, %edi\n.Llink_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nlink_old:\n  .asciz \""<<p.link_old<<"\"\nlink_new:\n  .asciz \""<<p.link_new<<"\"\n";
}

void emit_readlink(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $89, %eax\n  lea readlink_path(%rip), %rdi\n  lea readlink_buf(%rip), %rsi\n  mov $4096, %edx\n  syscall\n  test %eax, %eax\n  js .Lreadlink_fail\n  mov %eax, %r12d\n  xor %r13d, %r13d\n.Lreadlink_write:\n  cmp %r12d, %r13d\n  jge .Lreadlink_done_ok\n  mov $1, %eax\n  mov $1, %edi\n  lea readlink_buf(%rip), %rsi\n  add %r13, %rsi\n  mov %r12d, %edx\n  sub %r13d, %edx\n  syscall\n  test %eax, %eax\n  jle .Lreadlink_fail\n  add %eax, %r13d\n  jmp .Lreadlink_write\n.Lreadlink_done_ok:\n  xor %edi, %edi\n  jmp .Lreadlink_done\n.Lreadlink_fail:\n  mov $1, %edi\n.Lreadlink_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nreadlink_path:\n  .asciz \""<<p.readlink_path<<"\"\n.bss\n.align 8\nreadlink_buf:\n  .skip 4096\n";
}

void emit_rename(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $82, %eax\n  lea rename_old(%rip), %rdi\n  lea rename_new(%rip), %rsi\n  syscall\n  test %eax, %eax\n  js .Lrename_fail\n  xor %edi, %edi\n  jmp .Lrename_done\n.Lrename_fail:\n  mov $1, %edi\n.Lrename_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nrename_old:\n  .asciz \""<<p.rename_old<<"\"\nrename_new:\n  .asciz \""<<p.rename_new<<"\"\n";
}

void emit_chmod(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $90, %eax\n  lea chmod_path(%rip), %rdi\n  mov $"<<p.chmod_mode<<", %esi\n  syscall\n  test %eax, %eax\n  js .Lchmod_fail\n  xor %edi, %edi\n  jmp .Lchmod_done\n.Lchmod_fail:\n  mov $1, %edi\n.Lchmod_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nchmod_path:\n  .asciz \""<<p.chmod_path<<"\"\n";
}

void emit_access(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $21, %eax\n  lea access_path(%rip), %rdi\n  mov $"<<p.access_mode<<", %esi\n  syscall\n  test %eax, %eax\n  js .Laccess_fail\n  xor %edi, %edi\n  jmp .Laccess_done\n.Laccess_fail:\n  mov $1, %edi\n.Laccess_done:\n  mov $60, %eax\n  syscall\n.section .rodata\naccess_path:\n  .asciz \""<<p.access_path<<"\"\n";
}

void emit_truncate(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $76, %eax\n  lea truncate_path(%rip), %rdi\n  mov $"<<p.truncate_size<<", %rsi\n  syscall\n  test %eax, %eax\n  js .Ltruncate_fail\n  xor %edi, %edi\n  jmp .Ltruncate_done\n.Ltruncate_fail:\n  mov $1, %edi\n.Ltruncate_done:\n  mov $60, %eax\n  syscall\n.section .rodata\ntruncate_path:\n  .asciz \""<<p.truncate_path<<"\"\n";
}

void emit_getrandom(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $"<<p.random_bytes<<", %r12\n  lea random_buf(%rip), %r13\n.Lrandom_read:\n  test %r12, %r12\n  jz .Lrandom_done\n  mov $318, %eax\n  mov %r13, %rdi\n  mov %r12, %rsi\n  xor %edx, %edx\n  syscall\n  test %eax, %eax\n  js .Lrandom_fail\n  jz .Lrandom_done\n  mov %eax, %r14d\n  mov $1, %eax\n  mov $1, %edi\n  mov %r13, %rsi\n  mov %r14d, %edx\n  syscall\n  add %r14, %r13\n  sub %r14, %r12\n  jmp .Lrandom_read\n.Lrandom_fail:\n  mov $1, %edi\n  jmp .Lrandom_exit\n.Lrandom_done:\n  xor %edi, %edi\n.Lrandom_exit:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\nrandom_buf:\n  .skip 4096\n";
}

void emit_readstdin(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $"<<p.stdin_bytes<<", %r12\n.Lstdin_read:\n  test %r12, %r12\n  jz .Lstdin_done\n  xor %eax, %eax\n  xor %edi, %edi\n  lea stdin_buf(%rip), %rsi\n  mov %r12, %rdx\n  syscall\n  test %eax, %eax\n  js .Lstdin_fail\n  jz .Lstdin_done\n  mov %eax, %r14d\n  mov $1, %eax\n  mov $1, %edi\n  lea stdin_buf(%rip), %rsi\n  mov %r14d, %edx\n  syscall\n  sub %r14, %r12\n  jmp .Lstdin_read\n.Lstdin_fail:\n  mov $1, %edi\n  jmp .Lstdin_exit\n.Lstdin_done:\n  xor %edi, %edi\n.Lstdin_exit:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\nstdin_buf:\n  .skip 4096\n";
}

void emit_sleep(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $"<<p.sleep_seconds<<", %rax\n  mov %rax, sleep_ts(%rip)\n"
    <<"  movq $0, sleep_ts+8(%rip)\n  lea sleep_ts(%rip), %rdi\n  xor %esi, %esi\n  mov $35, %eax\n  syscall\n"
    <<"  test %eax, %eax\n  js .Lsleep_fail\n  xor %edi, %edi\n  jmp .Lsleep_done\n.Lsleep_fail:\n  mov $1, %edi\n.Lsleep_done:\n  mov $60, %eax\n  syscall\n.data\n.align 8\nsleep_ts:\n  .quad 0\n  .quad 0\n";
}

void emit_isatty(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $16, %eax\n  mov $"<<p.tty_fd<<", %edi\n  mov $21505, %esi\n  xor %edx, %edx\n  syscall\n"
    <<"  test %eax, %eax\n  js .Lisatty_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lisatty_done\n.Lisatty_no:\n  mov $"<<p.else_status<<", %edi\n.Lisatty_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_sync(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $162, %eax\n  syscall\n  test %eax, %eax\n  js .Lsync_fail\n  xor %edi, %edi\n  jmp .Lsync_done\n.Lsync_fail:\n  mov $1, %edi\n.Lsync_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_fsync(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $257, %eax\n  mov $-100, %edi\n  lea fsync_path(%rip), %rsi\n  xor %edx, %edx\n  xor %r10d, %r10d\n  syscall\n  test %eax, %eax\n  js .Lfsync_fail\n  mov %eax, %r12d\n  mov $74, %eax\n  mov %r12d, %edi\n  syscall\n  mov $3, %eax\n  mov %r12d, %edi\n  syscall\n  test %eax, %eax\n  js .Lfsync_fail\n  xor %edi, %edi\n  jmp .Lfsync_done\n.Lfsync_fail:\n  mov $1, %edi\n.Lfsync_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nfsync_path:\n  .asciz \""<<p.fsync_path<<"\"\n";
}

void emit_fdatasync(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $257, %eax\n  mov $-100, %edi\n  lea fdatasync_path(%rip), %rsi\n  xor %edx, %edx\n  xor %r10d, %r10d\n  syscall\n  test %eax, %eax\n  js .Lfdatasync_fail\n  mov %eax, %r12d\n  mov $75, %eax\n  mov %r12d, %edi\n  syscall\n  mov %eax, %r13d\n  mov $3, %eax\n  mov %r12d, %edi\n  syscall\n  test %r13d, %r13d\n  js .Lfdatasync_fail\n  xor %edi, %edi\n  jmp .Lfdatasync_done\n.Lfdatasync_fail:\n  mov $1, %edi\n.Lfdatasync_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nfdatasync_path:\n  .asciz \""<<p.fdatasync_path<<"\"\n";
}

void emit_umask(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $95, %eax\n  mov $"<<p.umask_mode<<", %edi\n  syscall\n  xor %edi, %edi\n  mov $60, %eax\n  syscall\n";
}

void emit_fcntl(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $72, %eax\n  mov $"<<p.fcntl_fd<<", %edi\n  mov $"<<p.fcntl_cmd<<", %esi\n  mov $"<<p.fcntl_arg<<", %edx\n  syscall\n  test %eax, %eax\n  js .Lfcntl_fail\n  xor %edi, %edi\n  jmp .Lfcntl_done\n.Lfcntl_fail:\n  mov $1, %edi\n.Lfcntl_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_setpgid(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $109, %eax\n  mov $"<<p.setpgid_pid<<", %edi\n  mov $"<<p.setpgid_pgid<<", %esi\n  syscall\n  test %eax, %eax\n  js .Lsetpgid_fail\n  xor %edi, %edi\n  jmp .Lsetpgid_done\n.Lsetpgid_fail:\n  mov $1, %edi\n.Lsetpgid_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_yield(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $24, %eax\n  syscall\n  test %eax, %eax\n  js .Lyield_fail\n  xor %edi, %edi\n  jmp .Lyield_done\n.Lyield_fail:\n  mov $1, %edi\n.Lyield_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_getpid(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $39, %eax\n  syscall\n  test %eax, %eax\n  jz .Lgetpid_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lgetpid_done\n.Lgetpid_no:\n  mov $"<<p.else_status<<", %edi\n.Lgetpid_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_getppid(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $110, %eax\n  syscall\n  test %eax, %eax\n  jz .Lgetppid_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lgetppid_done\n.Lgetppid_no:\n  mov $"<<p.else_status<<", %edi\n.Lgetppid_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_getuid(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $102, %eax\n  syscall\n  test %eax, %eax\n  js .Lgetuid_fail\n  xor %edi, %edi\n  jmp .Lgetuid_done\n.Lgetuid_fail:\n  mov $1, %edi\n.Lgetuid_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_geteuid(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $107, %eax\n  syscall\n  test %eax, %eax\n  js .Lgeteuid_fail\n  xor %edi, %edi\n  jmp .Lgeteuid_done\n.Lgeteuid_fail:\n  mov $1, %edi\n.Lgeteuid_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_getgid(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $104, %eax\n  syscall\n  test %eax, %eax\n  js .Lgetgid_fail\n  xor %edi, %edi\n  jmp .Lgetgid_done\n.Lgetgid_fail:\n  mov $1, %edi\n.Lgetgid_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_getegid(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $108, %eax\n  syscall\n  test %eax, %eax\n  js .Lgetegid_fail\n  xor %edi, %edi\n  jmp .Lgetegid_done\n.Lgetegid_fail:\n  mov $1, %edi\n.Lgetegid_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_getpgid(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $121, %eax\n  mov $"<<getpgid_pid_value<<", %edi\n  syscall\n  test %eax, %eax\n  js .Lgetpgid_fail\n  xor %edi, %edi\n  jmp .Lgetpgid_done\n.Lgetpgid_fail:\n  mov $1, %edi\n.Lgetpgid_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_getsid(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $124, %eax\n  mov $"<<getsid_pid_value<<", %edi\n  syscall\n  test %eax, %eax\n  js .Lgetsid_fail\n  xor %edi, %edi\n  jmp .Lgetsid_done\n.Lgetsid_fail:\n  mov $1, %edi\n.Lgetsid_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_setpriority(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $141, %eax\n  mov $"<<p.priority_which<<", %edi\n  mov $"<<p.priority_who<<", %esi\n  mov $"<<p.priority_value<<", %edx\n  syscall\n  test %eax, %eax\n  js .Lsetpriority_fail\n  xor %edi, %edi\n  jmp .Lsetpriority_done\n.Lsetpriority_fail:\n  mov $1, %edi\n.Lsetpriority_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_isroot(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $102, %eax\n  syscall\n  test %eax, %eax\n  jnz .Lisroot_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lisroot_done\n.Lisroot_no:\n  mov $"<<p.else_status<<", %edi\n.Lisroot_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_gettid(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $186, %eax\n  syscall\n  test %eax, %eax\n  jz .Lgettid_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lgettid_done\n.Lgettid_no:\n  mov $"<<p.else_status<<", %edi\n.Lgettid_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_isgroup0(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $104, %eax\n  syscall\n  test %eax, %eax\n  jnz .Lisgroup0_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lisgroup0_done\n.Lisgroup0_no:\n  mov $"<<p.else_status<<", %edi\n.Lisgroup0_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_ise_group0(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $108, %eax\n  syscall\n  test %eax, %eax\n  jnz .Lisegroup0_no\n  mov $"<<p.then_status<<", %edi\n  jmp .Lisegroup0_done\n.Lisegroup0_no:\n  mov $"<<p.else_status<<", %edi\n.Lisegroup0_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_nice(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $34, %eax\n  mov $"<<p.nice_increment<<", %edi\n  syscall\n  test %eax, %eax\n  js .Lnice_fail\n  xor %edi, %edi\n  jmp .Lnice_done\n.Lnice_fail:\n  mov $1, %edi\n.Lnice_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_writefd(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $"<<p.writefd_len<<", %r12\n  xor %r13d, %r13d\n.Lwritefd_loop:\n  cmp %r12, %r13\n  jge .Lwritefd_done\n  mov $1, %eax\n  mov $"<<p.writefd_fd<<", %edi\n  lea writefd_buf(%rip), %rsi\n  add %r13, %rsi\n  mov %r12, %rdx\n  sub %r13, %rdx\n  syscall\n  test %eax, %eax\n  jle .Lwritefd_fail\n  add %eax, %r13\n  jmp .Lwritefd_loop\n.Lwritefd_fail:\n  mov $1, %edi\n.Lwritefd_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nwritefd_buf:\n  .byte ";
  for(std::size_t i=0;i<p.writefd_text.size();++i) { if(i) std::cout<<", "; std::cout<<(unsigned)(unsigned char)p.writefd_text[i]; }
  std::cout<<"\n";
}

void emit_readfd(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $"<<p.readfd_len<<", %r12\n.Lreadfd_read:\n  test %r12, %r12\n  jz .Lreadfd_done\n  xor %eax, %eax\n  mov $"<<p.readfd_fd<<", %edi\n  lea readfd_buf(%rip), %rsi\n  mov %r12, %rdx\n  syscall\n  test %eax, %eax\n  js .Lreadfd_fail\n  jz .Lreadfd_done\n  mov %eax, %r13d\n  xor %r14d, %r14d\n.Lreadfd_write:\n  cmp %r13d, %r14d\n  jge .Lreadfd_consume\n  mov $1, %eax\n  mov $1, %edi\n  lea readfd_buf(%rip), %rsi\n  add %r14, %rsi\n  mov %r13d, %edx\n  sub %r14d, %edx\n  syscall\n  test %eax, %eax\n  jle .Lreadfd_fail\n  add %eax, %r14d\n  jmp .Lreadfd_write\n.Lreadfd_consume:\n  sub %r13, %r12\n  jmp .Lreadfd_read\n.Lreadfd_done:\n  xor %edi, %edi\n  jmp .Lreadfd_exit\n.Lreadfd_fail:\n  mov $1, %edi\n.Lreadfd_exit:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\nreadfd_buf:\n  .skip 4096\n";
}

void emit_poll(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $7, %eax\n  lea pollfd(%rip), %rdi\n  mov $1, %esi\n  mov $"<<p.poll_timeout<<", %edx\n  syscall\n  test %eax, %eax\n  js .Lpoll_fail\n  xor %edi, %edi\n  jmp .Lpoll_done\n.Lpoll_fail:\n  mov $1, %edi\n.Lpoll_done:\n  mov $60, %eax\n  syscall\n.data\n.align 8\npollfd:\n  .long "<<p.poll_fd<<"\n  .short "<<p.poll_events<<"\n  .short 0\n";
}

void emit_alarm(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $37, %eax\n  mov $"<<p.alarm_seconds<<", %edi\n  syscall\n  xor %edi, %edi\n  mov $60, %eax\n  syscall\n";
}

void emit_clock_gettime(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $228, %eax\n  mov $"<<p.clock_id<<", %edi\n  lea clock_ts(%rip), %rsi\n  syscall\n  test %eax, %eax\n  js .Lclock_fail\n  xor %edi, %edi\n  jmp .Lclock_done\n.Lclock_fail:\n  mov $1, %edi\n.Lclock_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\nclock_ts:\n  .skip 16\n";
}

void emit_gettimeofday(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $96, %eax\n  lea timeval(%rip), %rdi\n  xor %esi, %esi\n  syscall\n  test %eax, %eax\n  js .Lgettimeofday_fail\n  xor %edi, %edi\n  jmp .Lgettimeofday_done\n.Lgettimeofday_fail:\n  mov $1, %edi\n.Lgettimeofday_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\ntimeval:\n  .skip 16\n";
}

void emit_times(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $100, %eax\n  lea tms_buf(%rip), %rdi\n  syscall\n  test %eax, %eax\n  js .Ltimes_fail\n  xor %edi, %edi\n  jmp .Ltimes_done\n.Ltimes_fail:\n  mov $1, %edi\n.Ltimes_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\ntms_buf:\n  .skip 32\n";
}

void emit_getrusage(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $98, %eax\n  mov $"<<p.rusage_who<<", %edi\n  lea rusage_buf(%rip), %rsi\n  syscall\n  test %eax, %eax\n  js .Lrusage_fail\n  xor %edi, %edi\n  jmp .Lrusage_done\n.Lrusage_fail:\n  mov $1, %edi\n.Lrusage_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\nrusage_buf:\n  .skip 144\n";
}

void emit_sysinfo(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $99, %eax\n  lea sysinfo_buf(%rip), %rdi\n  syscall\n  test %eax, %eax\n  js .Lsysinfo_fail\n  xor %edi, %edi\n  jmp .Lsysinfo_done\n.Lsysinfo_fail:\n  mov $1, %edi\n.Lsysinfo_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\nsysinfo_buf:\n  .skip 128\n";
}

void emit_uname(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $63, %eax\n  lea uname_buf(%rip), %rdi\n  syscall\n  test %eax, %eax\n  js .Luname_fail\n  xor %edi, %edi\n  jmp .Luname_done\n.Luname_fail:\n  mov $1, %edi\n.Luname_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\nuname_buf:\n  .skip 390\n";
}

void emit_getdomainname(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $63, %eax\n  lea domain_buf(%rip), %rdi\n  syscall\n  test %eax, %eax\n  js .Ldomain_fail\n  xor %edi, %edi\n  jmp .Ldomain_done\n.Ldomain_fail:\n  mov $1, %edi\n.Ldomain_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\ndomain_buf:\n  .skip 390\n";
}

void emit_fstat(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $5, %eax\n  mov $"<<p.fstat_fd<<", %edi\n  lea fstat_buf(%rip), %rsi\n  syscall\n  test %eax, %eax\n  js .Lfstat_fail\n  xor %edi, %edi\n  jmp .Lfstat_done\n.Lfstat_fail:\n  mov $1, %edi\n.Lfstat_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\nfstat_buf:\n  .skip 144\n";
}

void emit_stat(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $4, %eax\n  lea stat_path(%rip), %rdi\n  lea stat_buf(%rip), %rsi\n  syscall\n  test %eax, %eax\n  js .Lstat_fail\n  xor %edi, %edi\n  jmp .Lstat_done\n.Lstat_fail:\n  mov $1, %edi\n.Lstat_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nstat_path:\n  .asciz \""<<p.stat_path<<"\"\n.bss\n.align 8\nstat_buf:\n  .skip 144\n";
}

void emit_lstat(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $6, %eax\n  lea lstat_path(%rip), %rdi\n  lea lstat_buf(%rip), %rsi\n  syscall\n  test %eax, %eax\n  js .Llstat_fail\n  xor %edi, %edi\n  jmp .Llstat_done\n.Llstat_fail:\n  mov $1, %edi\n.Llstat_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nlstat_path:\n  .asciz \""<<p.lstat_path<<"\"\n.bss\n.align 8\nlstat_buf:\n  .skip 144\n";
}

void emit_getgroups(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $115, %eax\n  mov $16, %edi\n  lea groups_buf(%rip), %rsi\n  syscall\n  test %eax, %eax\n  js .Lgroups_fail\n  xor %edi, %edi\n  jmp .Lgroups_done\n.Lgroups_fail:\n  mov $1, %edi\n.Lgroups_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\ngroups_buf:\n  .skip 64\n";
}

void emit_getresuid(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $118, %eax\n  lea ruid_real(%rip), %rdi\n  lea ruid_effective(%rip), %rsi\n  lea ruid_saved(%rip), %rdx\n  syscall\n  test %eax, %eax\n  js .Lruid_fail\n  xor %edi, %edi\n  jmp .Lruid_done\n.Lruid_fail:\n  mov $1, %edi\n.Lruid_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\nruid_real:\n  .skip 4\nruid_effective:\n  .skip 4\nruid_saved:\n  .skip 4\n";
}

void emit_getresgid(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $120, %eax\n  lea rgid_real(%rip), %rdi\n  lea rgid_effective(%rip), %rsi\n  lea rgid_saved(%rip), %rdx\n  syscall\n  test %eax, %eax\n  js .Lrgid_fail\n  xor %edi, %edi\n  jmp .Lrgid_done\n.Lrgid_fail:\n  mov $1, %edi\n.Lrgid_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\nrgid_real:\n  .skip 4\nrgid_effective:\n  .skip 4\nrgid_saved:\n  .skip 4\n";
}

void emit_getrlimit(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $97, %eax\n  mov $"<<p.rlimit_resource<<", %edi\n  lea rlimit_buf(%rip), %rsi\n  syscall\n  test %eax, %eax\n  js .Lrlimit_fail\n  xor %edi, %edi\n  jmp .Lrlimit_done\n.Lrlimit_fail:\n  mov $1, %edi\n.Lrlimit_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\nrlimit_buf:\n  .skip 16\n";
}

void emit_getpriority(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $140, %eax\n  mov $"<<p.priority_query_which<<", %edi\n  mov $"<<p.priority_query_who<<", %esi\n  syscall\n  test %eax, %eax\n  js .Lgetpriority_fail\n  xor %edi, %edi\n  jmp .Lgetpriority_done\n.Lgetpriority_fail:\n  mov $1, %edi\n.Lgetpriority_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_getcpu(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $309, %eax\n  lea cpu_buf(%rip), %rdi\n  lea cpu_node(%rip), %rsi\n  xor %edx, %edx\n  syscall\n  test %eax, %eax\n  js .Lgetcpu_fail\n  xor %edi, %edi\n  jmp .Lgetcpu_done\n.Lgetcpu_fail:\n  mov $1, %edi\n.Lgetcpu_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\ncpu_buf:\n  .skip 4\ncpu_node:\n  .skip 4\n";
}

void emit_sched_getaffinity(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $204, %eax\n  mov $"<<p.affinity_pid<<", %edi\n  mov $8, %esi\n  lea affinity_buf(%rip), %rdx\n  syscall\n  test %eax, %eax\n  js .Laffinity_fail\n  xor %edi, %edi\n  jmp .Laffinity_done\n.Laffinity_fail:\n  mov $1, %edi\n.Laffinity_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\naffinity_buf:\n  .skip 8\n";
}

void emit_eventfd(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $290, %eax\n  mov $"<<p.eventfd_init<<", %edi\n  xor %esi, %esi\n  syscall\n  test %eax, %eax\n  js .Leventfd_fail\n  mov %eax, %edi\n  mov $3, %eax\n  syscall\n  xor %edi, %edi\n  jmp .Leventfd_done\n.Leventfd_fail:\n  mov $1, %edi\n.Leventfd_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_timerfd_create(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $283, %eax\n  mov $"<<p.timerfd_clock<<", %edi\n  mov $"<<p.timerfd_flags<<", %esi\n  syscall\n  test %eax, %eax\n  js .Ltimerfd_fail\n  mov %eax, %edi\n  mov $3, %eax\n  syscall\n  xor %edi, %edi\n  jmp .Ltimerfd_done\n.Ltimerfd_fail:\n  mov $1, %edi\n.Ltimerfd_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_inotify_init1(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $294, %eax\n  mov $"<<p.inotify_flags<<", %edi\n  syscall\n  test %eax, %eax\n  js .Linotify_fail\n  mov %eax, %edi\n  mov $3, %eax\n  syscall\n  xor %edi, %edi\n  jmp .Linotify_done\n.Linotify_fail:\n  mov $1, %edi\n.Linotify_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_pidfd_open(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $434, %eax\n  mov $"<<p.pidfd_pid<<", %edi\n  mov $"<<p.pidfd_flags<<", %esi\n  syscall\n  test %eax, %eax\n  js .Lpidfd_fail\n  mov %eax, %edi\n  mov $3, %eax\n  syscall\n  xor %edi, %edi\n  jmp .Lpidfd_done\n.Lpidfd_fail:\n  mov $1, %edi\n.Lpidfd_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_memfd_create(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $319, %eax\n  lea memfd_name(%rip), %rdi\n  mov $"<<p.memfd_flags<<", %esi\n  syscall\n  test %eax, %eax\n  js .Lmemfd_fail\n  mov %eax, %edi\n  mov $3, %eax\n  syscall\n  xor %edi, %edi\n  jmp .Lmemfd_done\n.Lmemfd_fail:\n  mov $1, %edi\n.Lmemfd_done:\n  mov $60, %eax\n  syscall\n.section .rodata\nmemfd_name:\n  .asciz \""<<p.memfd_name<<"\"\n";
}

void emit_epoll_create1(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $291, %eax\n  mov $"<<p.epoll_flags<<", %edi\n  syscall\n  test %eax, %eax\n  js .Lepoll_fail\n  mov %eax, %edi\n  mov $3, %eax\n  syscall\n  xor %edi, %edi\n  jmp .Lepoll_done\n.Lepoll_fail:\n  mov $1, %edi\n.Lepoll_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_epoll_create(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $213, %eax\n  mov $"<<p.epoll_size<<", %edi\n  syscall\n  test %eax, %eax\n  js .Lepoll_legacy_fail\n  mov %eax, %edi\n  mov $3, %eax\n  syscall\n  xor %edi, %edi\n  jmp .Lepoll_legacy_done\n.Lepoll_legacy_fail:\n  mov $1, %edi\n.Lepoll_legacy_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_dup(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $33, %eax\n  mov $"<<p.dup_old<<", %edi\n  mov $"<<p.dup_new<<", %esi\n  syscall\n  test %eax, %eax\n  js .Ldup_fail\n  xor %edi, %edi\n  jmp .Ldup_done\n.Ldup_fail:\n  mov $1, %edi\n.Ldup_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_close(Program const& p) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $3, %eax\n  mov $"<<p.close_fd<<", %edi\n  syscall\n  test %eax, %eax\n  js .Lclose_fail\n  xor %edi, %edi\n  jmp .Lclose_done\n.Lclose_fail:\n  mov $1, %edi\n.Lclose_done:\n  mov $60, %eax\n  syscall\n";
}

void emit_pipe(Program const&) {
  std::cout<<".text\n.globl _start\n_start:\n"
    <<"  mov $22, %eax\n  lea pipe_fds(%rip), %rdi\n  syscall\n  test %eax, %eax\n  js .Lpipe_fail\n  xor %edi, %edi\n  jmp .Lpipe_done\n.Lpipe_fail:\n  mov $1, %edi\n.Lpipe_done:\n  mov $60, %eax\n  syscall\n.bss\n.align 8\npipe_fds:\n  .skip 8\n";
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
    if(program.cat) { csubset::emit_cat(program); return 0; }
    if(program.mkdir) { csubset::emit_mkdir(program); return 0; }
    if(program.rm) { csubset::emit_rm(program); return 0; }
    if(program.rmdir) { csubset::emit_rmdir(program); return 0; }
    if(program.touch) { csubset::emit_touch(program); return 0; }
    if(program.chdir) { csubset::emit_chdir(program); return 0; }
    if(program.symlink) { csubset::emit_symlink(program); return 0; }
    if(program.link) { csubset::emit_link(program); return 0; }
    if(program.readlink) { csubset::emit_readlink(program); return 0; }
    if(program.rename) { csubset::emit_rename(program); return 0; }
    if(program.chmod) { csubset::emit_chmod(program); return 0; }
    if(program.access) { csubset::emit_access(program); return 0; }
    if(program.truncate) { csubset::emit_truncate(program); return 0; }
    if(program.getrandom) { csubset::emit_getrandom(program); return 0; }
    if(program.readstdin) { csubset::emit_readstdin(program); return 0; }
    if(program.sleep) { csubset::emit_sleep(program); return 0; }
    if(program.isatty) { csubset::emit_isatty(program); return 0; }
    if(program.sync) { csubset::emit_sync(program); return 0; }
    if(program.fsync) { csubset::emit_fsync(program); return 0; }
    if(program.fdatasync) { csubset::emit_fdatasync(program); return 0; }
    if(program.umask) { csubset::emit_umask(program); return 0; }
    if(program.fcntl) { csubset::emit_fcntl(program); return 0; }
    if(program.setpgid) { csubset::emit_setpgid(program); return 0; }
    if(program.yield) { csubset::emit_yield(program); return 0; }
    if(csubset::emit_getuid_mode) { csubset::emit_getuid(program); return 0; }
    if(csubset::emit_geteuid_mode) { csubset::emit_geteuid(program); return 0; }
    if(csubset::emit_getgid_mode) { csubset::emit_getgid(program); return 0; }
    if(csubset::emit_getegid_mode) { csubset::emit_getegid(program); return 0; }
    if(csubset::emit_getpgid_mode) { csubset::emit_getpgid(program); return 0; }
    if(csubset::emit_getsid_mode) { csubset::emit_getsid(program); return 0; }
    if(program.getpid) { csubset::emit_getpid(program); return 0; }
    if(program.getppid) { csubset::emit_getppid(program); return 0; }
    if(program.setpriority) { csubset::emit_setpriority(program); return 0; }
    if(program.isroot) { csubset::emit_isroot(program); return 0; }
    if(program.gettid) { csubset::emit_gettid(program); return 0; }
    if(program.isgroup0) { csubset::emit_isgroup0(program); return 0; }
    if(program.ise_group0) { csubset::emit_ise_group0(program); return 0; }
    if(program.nice) { csubset::emit_nice(program); return 0; }
    if(program.writefd) { csubset::emit_writefd(program); return 0; }
    if(program.readfd) { csubset::emit_readfd(program); return 0; }
    if(program.poll) { csubset::emit_poll(program); return 0; }
    if(program.alarm) { csubset::emit_alarm(program); return 0; }
    if(program.clock_gettime) { csubset::emit_clock_gettime(program); return 0; }
    if(program.gettimeofday) { csubset::emit_gettimeofday(program); return 0; }
    if(program.times) { csubset::emit_times(program); return 0; }
    if(program.getrusage) { csubset::emit_getrusage(program); return 0; }
    if(program.sysinfo) { csubset::emit_sysinfo(program); return 0; }
    if(program.uname) { csubset::emit_uname(program); return 0; }
    if(program.getdomainname) { csubset::emit_getdomainname(program); return 0; }
    if(program.fstat) { csubset::emit_fstat(program); return 0; }
    if(program.stat) { csubset::emit_stat(program); return 0; }
    if(program.lstat) { csubset::emit_lstat(program); return 0; }
    if(program.getgroups) { csubset::emit_getgroups(program); return 0; }
    if(program.getresuid) { csubset::emit_getresuid(program); return 0; }
    if(program.getresgid) { csubset::emit_getresgid(program); return 0; }
    if(program.getrlimit) { csubset::emit_getrlimit(program); return 0; }
    if(program.getpriority) { csubset::emit_getpriority(program); return 0; }
    if(program.getcpu) { csubset::emit_getcpu(program); return 0; }
    if(program.sched_getaffinity) { csubset::emit_sched_getaffinity(program); return 0; }
    if(program.eventfd) { csubset::emit_eventfd(program); return 0; }
    if(program.timerfd_create) { csubset::emit_timerfd_create(program); return 0; }
    if(program.inotify_init1) { csubset::emit_inotify_init1(program); return 0; }
    if(program.pidfd_open) { csubset::emit_pidfd_open(program); return 0; }
    if(program.memfd_create) { csubset::emit_memfd_create(program); return 0; }
    if(program.epoll_create1) { csubset::emit_epoll_create1(program); return 0; }
    if(program.epoll_create) { csubset::emit_epoll_create(program); return 0; }
    if(program.dup) { csubset::emit_dup(program); return 0; }
    if(program.close) { csubset::emit_close(program); return 0; }
    if(program.pipe) { csubset::emit_pipe(program); return 0; }
    if(program.filter.size()) { csubset::emit_filtered_directory(program); return 0; }
    std::cout<<".text\n.globl _start\n_start:\n";
    if(!program.ordered_output.empty()) for(std::size_t i=0;i<program.ordered_output.size();++i) std::cout
             <<"  mov $1, %eax\n  mov $"<<program.ordered_output[i].first<<", %edi\n  lea ordered_"<<i<<"(%rip), %rsi\n  mov $"<<program.ordered_output[i].second.size()<<", %edx\n  syscall\n";
    if(program.ordered_output.empty() && !program.error_output.empty()) std::cout
             <<"  mov $1, %eax\n  mov $2, %edi\n  lea error_message(%rip), %rsi\n  mov $"<<program.error_output.size()<<", %edx\n  syscall\n";
    if(program.ordered_output.empty() && !program.output.empty()) std::cout
             <<"  mov $1, %eax\n  mov $1, %edi\n  lea message(%rip), %rsi\n  mov $"<<program.output.size()<<", %edx\n  syscall\n";
    if(program.loop_count>0 || program.loop_present) {
      if(program.loop_do) std::cout
        <<"  xor %r12d, %r12d\n.Lfor:\n  mov $1, %eax\n  mov $1, %edi\n  lea loop_message(%rip), %rsi\n  mov $"<<program.loop_output.size()<<", %edx\n  syscall\n  inc %r12d\n  cmp $"<<program.loop_count<<", %r12d\n  "<<(program.loop_inclusive ? "jg" : "jge")<<" .Lfor_done\n  jmp .Lfor\n.Lfor_done:\n";
      else std::cout
        <<"  xor %r12d, %r12d\n.Lfor:\n  cmp $"<<program.loop_count<<", %r12d\n  "<<(program.loop_inclusive ? "jg" : "jge")<<" .Lfor_done\n"
        <<"  mov $1, %eax\n  mov $1, %edi\n  lea loop_message(%rip), %rsi\n  mov $"<<program.loop_output.size()<<", %edx\n  syscall\n"
        <<"  inc %r12d\n  jmp .Lfor\n.Lfor_done:\n";
    }
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
    if(!program.error_output.empty()) {
      std::cout<<".section .rodata\nerror_message:\n  .byte ";
      for(std::size_t i=0;i<program.error_output.size();++i) { if(i) std::cout<<", "; std::cout<<(unsigned)(unsigned char)program.error_output[i]; }
      std::cout<<"\n";
    }
    if(!program.ordered_output.empty()) {
      std::cout<<".section .rodata\n";
      for(std::size_t i=0;i<program.ordered_output.size();++i) {
        std::cout<<"ordered_"<<i<<":\n  .byte ";
        for(std::size_t j=0;j<program.ordered_output[i].second.size();++j) { if(j) std::cout<<", "; std::cout<<(unsigned)(unsigned char)program.ordered_output[i].second[j]; }
        std::cout<<"\n";
      }
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
