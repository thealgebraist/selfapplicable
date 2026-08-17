int main(int argc, char **argv) {
  int i = 0;
  while (i <= 1) {
    write(1, "A", 1);
    write(1, "B", 1);
  }
  return 0;
}
