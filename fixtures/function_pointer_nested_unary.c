int identity(int x) { return x; }

int main(int argc, char **argv) {
  int (*fp)(int) = &identity;
  int (**pp)(int) = &fp;
  return (**pp)(6);
}
