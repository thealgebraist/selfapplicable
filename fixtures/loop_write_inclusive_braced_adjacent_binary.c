int main(int argc, char **argv) {
  for (int i = 0; i <= 1; i++) { write(1, "A\0" "\xFF", 3); }
  return 0;
}
