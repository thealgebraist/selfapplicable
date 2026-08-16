int add(int left, int right) { return left + right; }
int (*fp)(int, int) = &add;

int main(int argc, char **argv) {
  return (*fp)(1);
}
