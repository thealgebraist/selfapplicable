// A small find(1)-style CLI whose query is represented by total ADT data.
// The checker is the language's type-stage analogue: it validates the finite
// predicate before the filesystem stage is allowed to execute it.
#include <filesystem>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
namespace findadt {
namespace fs = std::filesystem;
struct True{}; struct Name{std::string pattern;}; struct Kind{char value;};
struct And; using P=std::variant<True,Name,Kind,std::shared_ptr<And>>;
struct And{P left,right;};
bool check(const P&p){return std::visit([](auto const&x)->bool{using X=std::decay_t<decltype(x)>;
  if constexpr(std::is_same_v<X,True>) return true;
  else if constexpr(std::is_same_v<X,Name>) return !x.pattern.empty();
  else if constexpr(std::is_same_v<X,Kind>) return x.value=='f'||x.value=='d'||x.value=='l';
  else return check(x->left)&&check(x->right);},p);}
std::string glob_regex(const std::string&g){std::string r="^";for(char c:g){if(c=='*')r+=".*";else if(c=='?')r+='.';else if(std::string(".\\+()[]{}^$|").find(c)!=std::string::npos){r+='\\';r+=c;}else r+=c;}return r+"$";}
bool eval(const P&p,const fs::directory_entry&e){return std::visit([&](auto const&x)->bool{using X=std::decay_t<decltype(x)>;
 if constexpr(std::is_same_v<X,True>)return true;
 else if constexpr(std::is_same_v<X,Name>)return std::regex_match(e.path().filename().string(),std::regex(glob_regex(x.pattern)));
 else if constexpr(std::is_same_v<X,Kind>){if(x.value=='f')return e.is_regular_file();if(x.value=='d')return e.is_directory();return e.is_symlink();}
 else return eval(x->left,e)&&eval(x->right,e);},p);}
P parse_query(int ac,char**av,int&maxdepth){P q=True{};std::vector<P>parts;maxdepth=-1;for(int i=2;i<ac;++i){std::string a=av[i];
 if(a=="-name"&&i+1<ac)parts.emplace_back(Name{av[++i]});
 else if(a=="-type"&&i+1<ac)parts.emplace_back(Kind{av[++i][0]});
 else if(a=="-maxdepth"&&i+1<ac)maxdepth=std::stoi(av[++i]);
 else if(a=="-print"){}
 else throw std::runtime_error("unsupported find option: "+a);}
 for(auto const&x:parts)q=std::make_shared<And>(And{q,x});
 return q;}
void walk(const fs::path&root,const P&q,int depth,int maxdepth){fs::directory_entry e(root);if(eval(q,e))std::cout<<root.string()<<'\n';if(!e.is_directory()||(maxdepth>=0&&depth>=maxdepth))return;for(auto const&x:fs::directory_iterator(root))walk(x.path(),q,depth+1,maxdepth);}
}
int main(int ac,char**av){try{if(ac<2){std::cerr<<"usage: find_adt ROOT [-name PATTERN] [-type f|d|l] [-maxdepth N]\n";return 2;}int depth;auto q=findadt::parse_query(ac,av,depth);if(!findadt::check(q))throw std::runtime_error("ill-typed predicate");findadt::walk(av[1],q,0,depth);return 0;}catch(std::exception const&e){std::cerr<<"find-adt: "<<e.what()<<'\n';return 2;}}
