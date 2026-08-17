int main(int argc, char **argv) {
  write(1, "A\x42\x43", 3);
  return 0;
}
