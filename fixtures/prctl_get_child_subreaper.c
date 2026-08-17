int main(int argc, char **argv) {
  prctl_get_child_subreaper();
  return 0;
}
