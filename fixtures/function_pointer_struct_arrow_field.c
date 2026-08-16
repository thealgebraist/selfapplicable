struct Box { int (*run)(int); };

int identity(int x) { return x; }

int main(int argc, char **argv) {
  struct Box box;
  struct Box *p = &box;
  p->run = &identity;
  return p->run(6);
}
