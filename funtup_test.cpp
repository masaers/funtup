#include "funtup.hpp"
#include <cassert>



struct add3 { int operator()(int a) const { return a + 3; } };
struct mul3 { int operator()(int a) const { return a * 3; } };
struct add { int operator()(int a, int b) const { return a + b; } };
struct mul { int operator()(int a, int b) const { return a * b; } };

int main(int argc, char** argv) {
  using namespace funtup;
  
  auto c1 = compose(add3(), mul3());
  auto c2 = compose(mul3(), add3());
  assert(c1(2) == 15);
  assert(c2(2) == 9);
  
  auto b = battery(add(), mul());
  std::tuple<int, int> r = b(3, 4);
  assert(std::get<0>(r) == 7);
  assert(std::get<1>(r) == 12);
  
  return 0;
}

