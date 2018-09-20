#include "masstree.h"
using namespace masstree;

Masstree::Masstree() {
  root_ = new BorderNode;
  root_->MarkIsRoot();
}

Masstree::~Masstree() {
  delete root_;
}

int main() {
  Node* border = new BorderNode;
  delete border;
  return 0;
}
