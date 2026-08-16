#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

// A self-applicable compiler instance for a small fixed-point C raytracer.
// The generated kernel traces one orthographic ray per pixel against a sphere.
namespace ray {
struct CAbi { std::string entry; std::string syscall_reg; int write_syscall; std::string svc; friend bool operator==(const CAbi&,const CAbi&)=default; };
struct RayDescription { CAbi abi; int width,height,cx,cy,radius; friend bool operator==(const RayDescription&,const RayDescription&)=default; };
template<class T> struct Code { T value; };
Code<RayDescription> quote(RayDescription x){return {std::move(x)};}
const RayDescription& unquote(const Code<RayDescription>& x){return x.value;}

RayDescription normalize(RayDescription x){
  if(x.width<=0||x.height<=0||x.radius<0)throw std::runtime_error("invalid raytracer description");
  return x;
}

struct Compiler {
  RayDescription d;
  std::string compile() const {
    auto x=normalize(d); std::string a=".text\n.global "+x.abi.entry+"\n";
    a+=x.abi.entry+":\n  stp x29, x30, [sp, #-16]!\n  mov x29, sp\n";
    a+="  mov x19, #0\n.Lrow:\n  cmp x19, #"+std::to_string(x.height)+"\n  b.ge .Lfinish\n  mov x20, #0\n.Lcolumn:\n  cmp x20, #"+std::to_string(x.width)+"\n  b.ge .Lnextrow\n  mov x0, x20\n  mov x1, x19\n  bl raytrace_pixel\n  mov x1, x20\n  mov x2, x19\n  bl pixel_sink\n  add x20, x20, #1\n  b .Lcolumn\n.Lnextrow:\n  add x19, x19, #1\n  b .Lrow\n.Lfinish:\n  ldp x29, x30, [sp], #16\n  ret\n\n";
    a+="// x0=x, x1=y; x0=1 on a sphere hit, else 0\nraytrace_pixel:\n  sub x2, x0, #"+std::to_string(x.cx)+"\n  mul x2, x2, x2\n  sub x3, x1, #"+std::to_string(x.cy)+"\n  mul x3, x3, x3\n  add x2, x2, x3\n  mov x3, #"+std::to_string(x.radius*x.radius)+"\n  cmp x2, x3\n  cset x0, lt\n  ret\n\n";
    a+="// Output policy: pixel_sink(intensity, x, y) is the C ABI hook.\n";
    a+="pixel_sink:\n  ret\n\n";
    a+="sys_write:\n  mov "+x.abi.syscall_reg+", #"+std::to_string(x.abi.write_syscall)+"\n  "+x.abi.svc+"\n  ret\n";
    return a;
  }
};
using Generator=std::function<Compiler(const RayDescription&)>;
Generator quoted_generator(){return [](const RayDescription& d){return Compiler{d};};}
std::string compile_staged(const Code<RayDescription>& code){return quoted_generator()(unquote(code)).compile();}
}

int main(){using namespace ray;try{
  RayDescription linuxRay{{"raytrace","x8",64,"svc #0"},64,48,32,24,20};
  auto quoted=quote(linuxRay);
  auto direct=Compiler{linuxRay}.compile();
  auto staged=compile_staged(quoted);
  if(direct!=staged)throw std::runtime_error("direct/staged compiler mismatch");
  std::cout<<"raytracer staged equivalence: PASS\n"<<direct;
}catch(std::exception const& e){std::cerr<<"error: "<<e.what()<<'\n';return 1;}}
