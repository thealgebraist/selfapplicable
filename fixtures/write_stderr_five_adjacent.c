int main(int argc, char **argv) {
  write(2, "A" "\0" "B" "\0" "C", 5);
  return 0;
}
