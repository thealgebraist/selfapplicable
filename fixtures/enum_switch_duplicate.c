enum Mode { First = 1, Second = 1 };

int main(int argc, char **argv) {
  switch (argc) {
    case First: return 7;
    case Second: return 8;
    default: return 3;
  }
}
