int main(int argc, char **argv) {
  if (isatty(0)) return 0;
  return 1;
}
