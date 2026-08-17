int main(int argc, char **argv) {
  int i = 0;
  while (i < 2) write(1, "A\0" "\xFF", 3); i++;
  return 0;
}
