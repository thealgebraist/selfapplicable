int main(int argc, char **argv) {
  int i = 0;
  do {
    write(1, "Z", 1);
    i++;
  } while (i < 0);
  return 0;
}
