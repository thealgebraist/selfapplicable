typedef int (*Unary)(int);

int identity(int x) { return x; }

int main(int argc, char **argv) {
  Unary fp = &identity;
  return fp(6);
}
