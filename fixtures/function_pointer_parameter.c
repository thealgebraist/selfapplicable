int identity(int x) { return x; }

int apply(int (*f)(int), int x) { return f(x); }

int main(int argc, char **argv) {
  return apply(&identity, 6);
}
