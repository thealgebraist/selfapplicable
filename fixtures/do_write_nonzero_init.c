int main(int argc, char **argv) {
  int i = 1;
  do {
    write(1, "U", 1);
    i++;
  } while (i < 2);
  return 0;
}
