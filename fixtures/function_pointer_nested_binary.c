int add(int x, int y) { return x + y; }

int main(int argc, char **argv) {
  int (*fp)(int, int) = &add;
  int (**pp)(int, int) = &fp;
  return (**pp)(2, 5);
}
