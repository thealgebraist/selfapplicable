int main(int argc, char **argv) {
  int i = 0;
  do {
    write(1, "U", 1);
    i += 1;
  } while (i <= 1);
  return 0;
}
