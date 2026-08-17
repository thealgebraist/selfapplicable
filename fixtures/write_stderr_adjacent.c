int main(int argc, char **argv) {
  write(2, "A" "\0", 2);
  return 0;
}
