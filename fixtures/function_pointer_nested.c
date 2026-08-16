int answer() { return 9; }

int main(int argc, char **argv) {
  int (*fp)() = &answer;
  int (**pp)() = &fp;
  return (**pp)();
}
