// A small find(1)-style CLI whose query is represented by total ADT data.
// The checker is the language's type-stage analogue: it validates the finite
// predicate before the filesystem stage is allowed to execute it.
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
namespace findadt {
namespace fs = std::filesystem;
struct True{}; struct Name{std::string pattern;}; struct Kind{char value;};
struct Size{char op; std::uintmax_t bytes;}; struct Empty{};
struct And; struct Or; struct Not;
using P=std::variant<True,Name,Kind,Size,Empty,std::shared_ptr<And>,std::shared_ptr<Or>,std::shared_ptr<Not>>;
struct And{P left,right;}; struct Or{P left,right;}; struct Not{P child;};
bool check(const P&p){return std::visit([](auto const&x)->bool{using X=std::decay_t<decltype(x)>;
  if constexpr(std::is_same_v<X,True>) return true;
  else if constexpr(std::is_same_v<X,Name>) return !x.pattern.empty();
  else if constexpr(std::is_same_v<X,Kind>) return x.value=='f'||x.value=='d'||x.value=='l';
  else if constexpr(std::is_same_v<X,Size>) return x.op=='+'||x.op=='-'||x.op=='=';
  else if constexpr(std::is_same_v<X,Empty>) return true;
  else if constexpr(std::is_same_v<X,std::shared_ptr<And>>||std::is_same_v<X,std::shared_ptr<Or>>) return check(x->left)&&check(x->right);
  else return check(x->child);},p);}
std::string glob_regex(const std::string&g){std::string r="^";for(char c:g){if(c=='*')r+=".*";else if(c=='?')r+='.';else if(std::string(".\\+()[]{}^$|").find(c)!=std::string::npos){r+='\\';r+=c;}else r+=c;}return r+"$";}
bool eval(const P&p,const fs::directory_entry&e){return std::visit([&](auto const&x)->bool{using X=std::decay_t<decltype(x)>;
 if constexpr(std::is_same_v<X,True>)return true;
 else if constexpr(std::is_same_v<X,Name>)return std::regex_match(e.path().filename().string(),std::regex(glob_regex(x.pattern)));
 else if constexpr(std::is_same_v<X,Kind>){if(x.value=='f')return e.is_regular_file();if(x.value=='d')return e.is_directory();return e.is_symlink();}
 else if constexpr(std::is_same_v<X,Size>){std::error_code ec;auto n=e.file_size(ec);if(ec)return false;return x.op=='+'?n>x.bytes:x.op=='-'?n<x.bytes:n==x.bytes;}
 else if constexpr(std::is_same_v<X,Empty>){std::error_code ec;return e.is_directory(ec)?fs::directory_iterator(e.path(),ec)==fs::directory_iterator():e.file_size(ec)==0;}
 else if constexpr(std::is_same_v<X,std::shared_ptr<And>>) return eval(x->left,e)&&eval(x->right,e);
 else if constexpr(std::is_same_v<X,std::shared_ptr<Or>>) return eval(x->left,e)||eval(x->right,e);
 else return !eval(x->child,e);},p);}
std::uintmax_t size_arg(std::string s){if(!s.empty()&&(s[0]=='+'||s[0]=='-'))s.erase(0,1);if(s.empty()||s.back()!='c')throw std::runtime_error("-size requires [+|-]BYTESc");s.pop_back();return std::stoull(s);}
P atom(int ac,char**av,int&i){std::string a=av[i];if(a=="-name"&&i+1<ac)return Name{av[++i]};if(a=="-type"&&i+1<ac)return Kind{av[++i][0]};if(a=="-size"&&i+1<ac){std::string s=av[++i];char op=(s[0]=='+'||s[0]=='-')?s[0]:'=';return Size{op,size_arg(s)};}if(a=="-empty")return Empty{};throw std::runtime_error("unsupported find expression: "+a);}
P parse_expr(int ac,char**av,int&maxdepth,int&mindepth){P q=True{};std::vector<P>parts;bool negate=false;bool disjoin=false;for(int i=2;i<ac;++i){std::string a=av[i];
 if(a=="-maxdepth"&&i+1<ac){maxdepth=std::stoi(av[++i]);continue;}if(a=="-mindepth"&&i+1<ac){mindepth=std::stoi(av[++i]);continue;}if(a=="-print")continue;if(a=="!"||a=="-not"){negate=!negate;continue;}if(a=="-o"||a=="-or"){disjoin=true;continue;}auto x=atom(ac,av,i);if(negate){x=std::make_shared<Not>(Not{x});negate=false;}parts.push_back(std::move(x));}
 if(parts.empty())return q;
 q=parts.front();for(size_t i=1;i<parts.size();++i)q=disjoin?P{std::make_shared<Or>(Or{q,parts[i]})}:P{std::make_shared<And>(And{q,parts[i]})};return q;}
void walk(const fs::path&root,const P&q,int depth,int maxdepth,int mindepth){std::error_code ec;fs::directory_entry e(root,ec);if(ec)throw std::runtime_error("cannot access "+root.string());if(depth>=mindepth&&eval(q,e))std::cout<<root.string()<<'\n';if(!e.is_directory(ec)||(maxdepth>=0&&depth>=maxdepth))return;for(auto const&x:fs::directory_iterator(root,ec))if(!ec)walk(x.path(),q,depth+1,maxdepth,mindepth);}
}
int main(int ac,char**av){try{if(ac<2){std::cerr<<"usage: find_adt ROOT [-name PATTERN] [-type f|d|l] [-size [+|-]BYTESc] [-empty] [-not] [-o] [-maxdepth N] [-mindepth N]\n";return 2;}int maxdepth=-1,mindepth=0;auto q=findadt::parse_expr(ac,av,maxdepth,mindepth);if(!findadt::check(q))throw std::runtime_error("ill-typed predicate");findadt::walk(av[1],q,0,maxdepth,mindepth);return 0;}catch(std::exception const&e){std::cerr<<"find-adt: "<<e.what()<<'\n';return 2;}}
