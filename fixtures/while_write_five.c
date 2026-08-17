int main(int argc, char **argv) {
  int i = 0;
  while (i < 2) write(1, "A" "B" "\0" "\xFF" "C", 5);
  return 0;
}
