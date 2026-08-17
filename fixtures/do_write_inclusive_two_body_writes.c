int main(int argc, char **argv) {
  int i = 0;
  do {
    write(1, "A", 1);
    write(1, "\0", 1);
    i++;
  } while (i <= 1);
  return 0;
}
