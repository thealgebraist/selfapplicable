/* Macro-free C-subset compatibility fixture derived from true(1). */
int main(int argc, char **argv) {
  if (argc == 2 && streq(argv[1], "--help")) return 0;
  if (argc == 2) return 0;
  return 0;
}
