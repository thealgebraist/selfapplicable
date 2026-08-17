int main(int argc, char **argv) {
  int i = 0;
  while (i < 2) write(1, "A" "B" "\0" "\xFF", 4);
  return 0;
}
