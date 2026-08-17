int main(int argc, char **argv) {
  write(1, "A" "\0" "\xFF" "B", 4);
  return 0;
}
