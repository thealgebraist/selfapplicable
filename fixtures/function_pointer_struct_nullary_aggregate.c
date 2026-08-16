struct Box { int (*run)(); };

int answer() { return 9; }

int main(int argc, char **argv) {
  struct Box box = { &answer };
  return box.run();
}
