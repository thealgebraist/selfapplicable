int identity(int value) { return value; }

int main(int argc, char **argv) {
  int (*fp)(int) = &identity;
  int (*gp)(int) = fp;
  return gp(6);
}
