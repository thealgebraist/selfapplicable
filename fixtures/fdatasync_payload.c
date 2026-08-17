int main(int argc, char **argv) {
  fdatasync("fixtures/cat_payload.txt");
  return 0;
}
