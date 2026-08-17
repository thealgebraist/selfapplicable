int main(int argc, char **argv) {
  write(1, "\xGG" "A", 2);
  return 0;
}
