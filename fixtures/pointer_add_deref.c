int main(int argc, char **argv) {
  int values[3] = {4, 17, 8};
  int *p = values;
  int *q = p + 1;
  return *q;
}
