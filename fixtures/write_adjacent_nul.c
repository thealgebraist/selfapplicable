int main(int argc, char **argv) {
  write(1, "A\0" "B", 3);
  return 0;
}
