int main(int argc, char **argv) {
  for (int i = 0; i < 2; i++) write(1, "\x41", 1);
  return 0;
}
