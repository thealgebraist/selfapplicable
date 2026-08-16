int main(int argc, char **argv) {
  char buf[4096];
  write(1, getcwd(buf, 4096), strlen(buf));
  return 0;
}
