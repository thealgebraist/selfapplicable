int main(int argc, char **argv) {
  write(1, "A" "\0" "\xFF", 3);
  return 0;
}
