struct Box { int (*run)(int, int); };

int add(int x, int y) { return x + y; }

int main(int argc, char **argv) {
  struct Box box;
  box.run = &add;
  return box.run(2, 5);
}
