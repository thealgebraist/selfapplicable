int add(int x, int y) { return x + y; }

int apply2(int (*f)(int, int), int x, int y) { return f(x, y); }

int main(int argc, char **argv) {
  return apply2(&add, 2, 5);
}
