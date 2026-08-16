int main(int argc, char **argv) {
  int values[3] = {3, 15, 6};
  int *p = values;
  p[1] = 16;
  return p[1];
}
