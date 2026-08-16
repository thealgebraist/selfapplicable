struct Box { void (*run)(int); };

void consume(int x) { return; }

int main(int argc, char **argv) {
  struct Box box;
  struct Box *p = &box;
  p->run = &consume;
  p->run(6);
  return 0;
}
