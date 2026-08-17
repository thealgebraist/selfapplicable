int main(int argc, char **argv) {
  for (int i = 0; i <= 1; i++) {
    write(1, "A", 1);
    write(1, "\0", 1);
    write(1, "B", 1);
  }
  return 0;
}
