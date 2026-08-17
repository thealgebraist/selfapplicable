int main(int argc, char **argv) {
  for (int i = 0; i < 5; i++) {
    if (i == 2) break;
    if (i == 3) continue;
    write(1, "X", 1);
  }
  return 0;
}
