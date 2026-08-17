int main(int argc, char **argv) {
  int i = 0;
  while (i <= 1) write(1, "A" "\0" "\xFF", 3);
  return 0;
}
