void consume(int x, int y) { return; }

void invoke2(void (*f)(int, int), int x, int y) { f(x, y); }

int main(int argc, char **argv) {
  invoke2(&consume, 2, 5);
  return 0;
}
