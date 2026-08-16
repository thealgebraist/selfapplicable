struct Box { int (*run)(int); };

int identity(int x) { return x; }

int main(int argc, char **argv) {
  struct Box box = { &identity };
  return box.run(6);
}
