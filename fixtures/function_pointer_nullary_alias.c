int answer() { return 9; }

int main(int argc, char **argv) {
  int (*fp)() = &answer;
  int (*gp)() = fp;
  return gp();
}
