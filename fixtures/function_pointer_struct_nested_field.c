struct Box { int (**run)(int); };

int identity(int x) { return x; }

int main(int argc, char **argv) {
  struct Box box;
  int (*fp)(int) = &identity;
  box.run = &fp;
  return (**box.run)(6);
}
