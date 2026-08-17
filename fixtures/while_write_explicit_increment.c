int main(int argc, char **argv) {
  int i = 0;
  while (i < 3) {
    write(1, "R", 1);
    i++;
  }
  return 0;
}
