int main(int argc, char **argv) {
  int i = 0;
  while (i < 2) {
    write(1, "Q" "\0", 2);
  }
  return 0;
}
