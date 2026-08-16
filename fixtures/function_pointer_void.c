void ping() { return; }

int main(int argc, char **argv) {
  void (*fp)() = &ping;
  fp();
  return 0;
}
