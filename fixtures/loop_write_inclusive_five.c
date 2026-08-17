int main(int argc, char **argv) {
  for (int i = 0; i <= 1; i++) write(1, "A" "B" "\0" "\xFF" "C", 5);
  return 0;
}
