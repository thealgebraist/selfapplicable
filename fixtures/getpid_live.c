int main(int argc, char **argv) {
  if (getpid()) return 0;
  return 1;
}
