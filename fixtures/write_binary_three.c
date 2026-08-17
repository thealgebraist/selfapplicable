int main(int argc, char **argv) {
  write(1, "" "\0" "\xFF", 2);
  return 0;
}
