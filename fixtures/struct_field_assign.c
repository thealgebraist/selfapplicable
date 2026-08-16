struct Node {
  int value;
  struct Node *next;
};

int main(int argc, char **argv) {
  struct Node node = {0, 0};
  node.value = 6;
  return node.value;
}
