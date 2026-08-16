int identity(int value) { return value; }

int main(int argc, char **argv) {
  int (*fp)(int) = &identity;
  int (*gp)(int, int) = fp;
  return gp(2, 5);
}
