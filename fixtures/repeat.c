int main(int argc, char **argv) {
  for (int i = 0; i < 3; i++) write(1, "x\n", 2);
  return 0;
}
