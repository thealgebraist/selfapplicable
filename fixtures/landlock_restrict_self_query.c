int main(int argc, char **argv) {
  landlock_restrict_self_query();
  return 0;
}
