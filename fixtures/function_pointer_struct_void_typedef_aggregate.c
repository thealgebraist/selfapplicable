typedef void (*Action)(int);

struct Box { Action run; };

void consume(int x) { return; }

int main(int argc, char **argv) {
  struct Box box = { &consume };
  box.run(6);
  return 0;
}
