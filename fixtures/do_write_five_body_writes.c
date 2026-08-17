int main(int argc, char **argv) {
  int i = 0;
  do {
    write(1, "A", 1);
    write(1, "B", 1);
    write(1, "\0", 1);
    write(1, "\xFF", 1);
    write(1, "C", 1);
    i++;
  } while (i < 2);
  return 0;
}
