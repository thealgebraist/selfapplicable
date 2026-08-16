int add(int left, int right) { return left + right; }

int main(int argc, char **argv) {
  int (*fp)(int, int) = &add;
  return (*fp)(2, 5);
}
