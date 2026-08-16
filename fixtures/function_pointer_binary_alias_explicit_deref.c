int add(int left, int right) { return left + right; }

int main(int argc, char **argv) {
  int (*fp)(int, int) = &add;
  int (*gp)(int, int) = fp;
  return (*gp)(2, 5);
}
