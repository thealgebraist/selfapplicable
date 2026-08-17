int main(int argc, char **argv) {
  for (int i = 0; i < 2; i++) write(1, "A" "\0" "\xFF" "B", 4);
  return 0;
}
