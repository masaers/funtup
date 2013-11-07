#include "funtup.hpp"
#include <cassert>



struct add3 { int operator()(int a) const { return a + 3; } };
struct mul3 { int operator()(int a) const { return a * 3; } };
struct add { int operator()(int a, int b) const { return a + b; } };
struct mul { int operator()(int a, int b) const { return a * b; } };

std::tuple<int, int> divint(int a, int b) {
  return std::make_tuple(a / b, a % b);
}

int main(int argc, char** argv) {
  using namespace funtup;
  using namespace std;
  
  auto c1 = compose(add3(), mul3());
  auto c2 = compose(mul3(), add3());
  assert(c1(2) == 15);
  assert(c2(2) == 9);
  
  auto b = battery(add(), mul());
  auto r = b(3, 4);
  assert(get<0>(r) == 7);
  assert(get<1>(r) == 12);

  auto a = auto_unpack(add());
  assert(a(make_tuple(2, 1)) == 3);
  
  auto c3 = compose(&divint, auto_unpack(add()));
  assert(c3(5, 2) == 3);
  
  return 0;
}

