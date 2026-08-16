struct Node {
  int value;
  struct Node *next;
};

int main(int argc, char **argv) {
  struct Node node = {7, 0};
  return node.value;
}
