struct Pair { int (*first)(int); int (*second)(int); };

int identity(int x) { return x; }
int same(int x) { return x; }

int main(int argc, char **argv) {
  struct Pair pair;
  pair.first = &identity;
  pair.second = &same;
  return pair.second(6);
}
