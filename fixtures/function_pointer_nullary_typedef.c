typedef int (*Thunk)();

int answer() { return 9; }

int main(int argc, char **argv) {
  Thunk fp = &answer;
  return fp();
}
