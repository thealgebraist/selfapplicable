typedef int (*Unary)(int);

int identity(int x) { return x; }

int apply(Unary f, int x) { return f(x); }

int main(int argc, char **argv) {
  return apply(&identity, 6);
}
