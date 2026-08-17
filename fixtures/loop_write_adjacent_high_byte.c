int main(int argc, char **argv) {
  for (int i = 0; i < 2; i++) write(1, "\xFF" "A", 2);
  return 0;
}
