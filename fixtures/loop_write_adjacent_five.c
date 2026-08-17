int main(int argc, char **argv) {
  for (int i = 0; i < 2; i++) write(1, "A" "B" "C" "D" "E", 5);
  return 0;
}
