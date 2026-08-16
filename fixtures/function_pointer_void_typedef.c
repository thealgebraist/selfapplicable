typedef void (*Action)();

void ping() { return; }

int main(int argc, char **argv) {
  Action fp = &ping;
  fp();
  return 0;
}
