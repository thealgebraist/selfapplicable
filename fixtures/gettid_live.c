int main(int argc, char **argv) {
  if (gettid()) return 0;
  return 1;
}
