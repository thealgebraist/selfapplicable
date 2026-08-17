int main(int argc, char **argv) {
  for (int i = 0; i < 2; i++) write(1, "\x", 1);
  return 0;
}
