// Separate compiler front end for the Find ADT DSL.  It parses source data,
// checks the resulting finite ADT, and only then invokes the filesystem backend.
#define FIND_ADT_NO_MAIN
#include "minimal_find_cli.cpp"
#include <fstream>
#include <sstream>
namespace finddsl {
struct Parser { std::vector<std::string> t; size_t i=0;
  explicit Parser(std::string s){std::string w;bool quote=false;for(char c:s){if(c=='\"'){quote=!quote;continue;}if(!quote&&(c=='('||c==')')){if(!w.empty()){t.push_back(w);w.clear();}t.emplace_back(1,c);}else if(!quote&&std::isspace((unsigned char)c)){if(!w.empty()){t.push_back(w);w.clear();}}else w+=c;}if(!w.empty())t.push_back(w);}
  std::string take(){if(i>=t.size())throw std::runtime_error("unexpected end of Find DSL");return t[i++];}
  void open(){if(take()!="(")throw std::runtime_error("expected '('");}
  findadt::P expr(){open();auto k=take();if(k=="prune"){if(take()!=")")throw std::runtime_error("bad prune");return findadt::Prune{};}if(k=="empty"){if(take()!=")")throw std::runtime_error("bad empty");return findadt::Empty{};}if(k=="name"||k=="iname"){auto x=take();if(take()!=")")throw std::runtime_error("bad name");return findadt::Name{x,k=="iname"};}if(k=="path"){auto x=take();if(take()!=")")throw std::runtime_error("bad path");return findadt::Path{x};}if(k=="type"){auto x=take();if(take()!=")")throw std::runtime_error("bad type");return findadt::Kind{x[0]};}if(k=="size"){auto x=take();if(take()!=")")throw std::runtime_error("bad size");char op=(x[0]=='+'||x[0]=='-')?x[0]:'=';return findadt::Size{op,findadt::size_arg(x)};}if(k=="not"){auto x=expr();if(take()!=")")throw std::runtime_error("bad not");return std::make_shared<findadt::Not>(findadt::Not{x});}if(k=="and"||k=="or"){auto a=expr(),b=expr();if(take()!=")")throw std::runtime_error("bad boolean");return k=="and"?findadt::P{std::make_shared<findadt::And>(findadt::And{a,b})}:findadt::P{std::make_shared<findadt::Or>(findadt::Or{a,b})};}throw std::runtime_error("unknown Find DSL constructor: "+k);}
  std::pair<std::string,findadt::P> program(){open();if(take()!="find")throw std::runtime_error("program must start with find");auto root=take();auto q=expr();if(take()!=")"||i!=t.size())throw std::runtime_error("trailing Find DSL input");return {root,q};}
};
}
int main(int ac,char**av){try{if(ac!=2){std::cerr<<"usage: find_dsl_compiler PROGRAM.find\n";return 2;}std::ifstream f(av[1]);if(!f)throw std::runtime_error("cannot open Find DSL program");std::ostringstream s;s<<f.rdbuf();finddsl::Parser p(s.str());auto [root,q]=p.program();if(!findadt::check(q))throw std::runtime_error("Find DSL type stage rejected the ADT");std::cerr<<"Find DSL: checked and executing normalized ADT\n";findadt::walk(root,q,0,-1,0,false);return 0;}catch(std::exception const&e){std::cerr<<"find-dsl: "<<e.what()<<'\n';return 2;}}
