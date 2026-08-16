int countdown(int n) {
  if (n == 0) return 0;
  return countdown(n - 1);
}

int main(int argc, char **argv) {
  return countdown(0);
}
