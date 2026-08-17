int main(int argc, char **argv) {
  write(2, "A" "\0" "B" "\0", 4);
  return 0;
}
