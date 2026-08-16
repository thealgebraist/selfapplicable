typedef int (*Binary)(int, int);

int add(int x, int y) { return x + y; }

int main(int argc, char **argv) {
  Binary fp = &add;
  return fp(2, 5);
}
