/* Macro-free C-subset compatibility fixture: status-only echo skeleton. */
int main(int argc, char **argv) {
  write(1, "echo\n", 5);
  return 0;
}
