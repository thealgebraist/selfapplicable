int main(int argc, char **argv) {
  if (getppid()) return 0;
  return 1;
}
