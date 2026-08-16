int identity(int x) { return x; }

int main(int argc, char **argv) {
  int (*a)(int) = &identity;
  int (*b)(int) = &identity;
  if (a == b) return 1;
  return 0;
}
