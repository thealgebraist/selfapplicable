struct Node {
  int value;
  struct Node *next;
};

int main(int argc, char **argv) {
  struct Node node = {12, 0};
  node.next = &node;
  return node.next->value;
}
