struct Mix { int (*get)(int); void (*put)(int); };

int identity(int x) { return x; }
void consume(int x) { return; }

int main(int argc, char **argv) {
  struct Mix mix;
  mix.get = &identity;
  mix.put = &consume;
  mix.put(3);
  return mix.get(6);
}
