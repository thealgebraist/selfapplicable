int main(int argc, char **argv) {
  getxattr("/tmp", "user.selfapp");
  return 0;
}
