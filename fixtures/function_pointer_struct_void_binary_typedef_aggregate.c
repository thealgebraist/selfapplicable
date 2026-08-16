typedef void (*Action2)(int, int);

struct Box { Action2 run; };

void consume(int x, int y) { return; }

int main(int argc, char **argv) {
  struct Box box = { &consume };
  box.run(2, 5);
  return 0;
}
