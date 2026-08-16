int answer() { return 9; }
int (*fp)() = &answer;

int main(int argc, char **argv) {
  return (*fp)();
}
