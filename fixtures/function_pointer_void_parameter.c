void ping() { return; }

void invoke(void (*f)()) { f(); }

int main(int argc, char **argv) {
  invoke(&ping);
  return 0;
}
