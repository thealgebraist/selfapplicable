int main(int argc, char **argv) {
  write(1, "A" "B" "\0" "\xFF", 4);
  return 0;
}
