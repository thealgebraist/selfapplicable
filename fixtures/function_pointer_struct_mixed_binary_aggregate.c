struct Mix { int (*get)(int, int); void (*put)(int, int); };

int add(int x, int y) { return x + y; }
void consume(int x, int y) { return; }

int main(int argc, char **argv) {
  struct Mix mix = { &add, &consume };
  mix.put(3, 4);
  return mix.get(2, 5);
}
