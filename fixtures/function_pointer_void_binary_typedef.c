typedef void (*Action2)(int, int);

void consume(int x, int y) { return; }

int main(int argc, char **argv) {
  Action2 fp = &consume;
  fp(2, 5);
  return 0;
}
