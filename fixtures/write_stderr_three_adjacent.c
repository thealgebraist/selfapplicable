int main(int argc, char **argv) {
  write(2, "A" "\0" "B", 3);
  return 0;
}
