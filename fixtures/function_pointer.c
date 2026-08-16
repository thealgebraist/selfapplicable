int identity(int value) { return value; }

int main(int argc, char **argv) {
  int (*fp)(int) = identity;
  return fp(6);
}
