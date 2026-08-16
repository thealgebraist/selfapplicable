void consume(int x, int y) { return; }

int main(int argc, char **argv) {
  void (*fp)(int, int) = &consume;
  fp(2, 5);
  return 0;
}
