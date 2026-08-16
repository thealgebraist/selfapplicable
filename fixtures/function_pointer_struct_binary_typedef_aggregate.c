typedef int (*Binary)(int, int);

struct Box { Binary run; };

int add(int x, int y) { return x + y; }

int main(int argc, char **argv) {
  struct Box box = { &add };
  return box.run(2, 5);
}
