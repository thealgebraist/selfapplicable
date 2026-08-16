int identity(int value) { return value; }
int (*fp)(int) = &identity;

int main(int argc, char **argv) {
  return (*fp)(6);
}
