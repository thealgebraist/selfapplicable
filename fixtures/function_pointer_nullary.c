int answer() { return 9; }

int main(int argc, char **argv) {
  int (*fp)() = &answer;
  return fp();
}
