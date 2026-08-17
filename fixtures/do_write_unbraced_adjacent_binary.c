int main(int argc, char **argv) {
  int i = 0;
  do write(1, "A\0" "\xFF", 2); i++; while (i < 2);
  return 0;
}
