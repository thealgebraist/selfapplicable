int main(int argc, char **argv) {
  for (int i = 0; i < 5; i++) {
    if (i == 2) continue;
    if (i == 4) break;
    write(1, "X", 1);
  }
  return 0;
}
