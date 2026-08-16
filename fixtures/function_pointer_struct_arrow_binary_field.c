struct Box { int (*run)(int, int); };

int add(int x, int y) { return x + y; }

int main(int argc, char **argv) {
  struct Box box;
  struct Box *p = &box;
  p->run = &add;
  return p->run(2, 5);
}
