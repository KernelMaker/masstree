#ifndef MASSTREE_H_
#define MASSTREE_H_

#include <cstdint>
#include <atomic>
namespace masstree {
static const int32_t kNodeMax = 15;
// Version
static const uint32_t kNodeLocked = (1U << 0);
static const uint32_t kNodeInserting = (1U << 1);
static const uint32_t kNodeSpliting = (1U << 2);
static const uint32_t kNodeDeleted = (1U << 3);
static const uint32_t kNodeIsRoot = (1U << 4);
static const uint32_t kNodeIsBorder = (1U << 5);
static const uint32_t kNodeUnused = (1U << 31);
static const uint32_t kNodeVinsert = 0x00001fc0;
static const uint32_t kNodeVinsertShift = 6;
static const uint32_t kNodeVinsertLowbit = (1U << kNodeVinsertShift);
static const uint32_t kNodeVsplit = 0x7fffe000;
static const uint32_t kNodeVsplitShift = 13;
static const uint32_t kNodeVsplitLowbit = (1U << kNodeVsplitShift);

static const uint32_t kNodeDirty = kNodeInserting | kNodeSpliting;

// Key
static const uint8_t kKeyTypeValue = 0x00;
static const uint8_t kKeyTypeLayer = 0x40;
static const uint8_t kKeyTypeUnstable = 0x80;
static const uint8_t kKeyTypeNotFound = 0xff;
static inline uint8_t KeyLength(uint8_t key_len) {
  return key_len & 0x7f;
}
static inline uint8_t KeyType(uint8_t key_len) {
  return key_len & 0xc0;
}

// Permutation
static const uint64_t kPermutationInit = 0xedcba98765432100ULL;
static inline uint64_t NumKeys(uint64_t permutation) {
  return (permutation & 0xf);
}
static inline uint64_t IndexKey(uint64_t permutation, int index) {
  return (((permutation) >> (((index) * 4) + 4)) & 0xf);
}

class Node {
 public:
  explicit Node(uint32_t version) : version_(version) {
  };
  uint32_t version() {
    return version_;
  }
  bool Locked() {
    return version_ & kNodeLocked;
  }
  bool Locked(const uint32_t version) {
    return version & kNodeLocked;
  }
  void MarkLocked() {
    version_ |= kNodeLocked;
    asm volatile("" : : : "memory");
  }

  bool Inserting() {
    return version_ & kNodeInserting;
  }
  void MarkInserting() {
    version_ |= kNodeInserting;
    asm volatile("" : : : "memory");
  }

  bool Spliting() {
    return version_ & kNodeSpliting;
  }
  bool MarkSpliting() {
    version_ |= kNodeSpliting;
    asm volatile("" : : : "memory");
  }
  bool HasSplited(uint32_t version) {
    asm volatile("" : : : "memory");
    return (version_ ^ version) > kNodeVsplitLowbit;
  }

  bool Deleted() {
    return version_ & kNodeDeleted;
  }
  void MarkDeleted() {
    version_ |= kNodeDeleted;
    // version_ |= kNodeDeleted | kNodeSpliting;
    asm volatile("" : : : "memory");
  }

  bool IsRoot() {
    return version_ & kNodeIsRoot;
  }
  void MarkIsRoot() {
    version_ |= kNodeIsRoot;
    asm volatile("" : : : "memory");
  }
  void UnmarkIsRoot() {
    version_ |= ~kNodeIsRoot;
    asm volatile("" : : : "memory");
  }

  bool IsBorder() {
    return version_ & kNodeIsBorder;
  }
  void MarkIsBorder() {
    version_ |= kNodeIsBorder;
    asm volatile("" : : : "memory");
  }
  void UnmarkIsBorder() {
    version_ |= ~kNodeIsBorder;
    asm volatile("" : : : "memory");
  }

  void Lock() {
    uint32_t v;
    v = version_.load();
    while (true) {
      if (Locked(v)) {
        asm volatile("pause" : : : "memory");
        continue;
      };
      while (!version_.compare_exchange_weak(v, v | kNodeLocked)) {
        continue;
      }
      asm volatile("" : : : "memory");
      break;
    }
  }
  void UnLock() {
    uint32_t v = version_.load();
    if (Inserting()) {
      v = (v & ~kNodeVinsert) | ((v & kNodeVinsert) + (1 << kNodeVinsertShift));
    }
    if (Spliting()) {
      v = ((v & ~kNodeIsRoot) & ~kNodeVsplit) |
            (((v & kNodeVsplit) + (1 << kNodeVsplitShift)) & kNodeVsplit);
    }

    v &= ~(kNodeLocked | kNodeInserting | kNodeSpliting);
    version_.store(v);
  }

 private:
  std::atomic<uint32_t> version_;
};

class InteriorNode : public Node {
 public:
  InteriorNode() : Node(0) {
  };
 private:
  uint8_t n_keys_;
  uint64_t key_slice_[kNodeMax];
  Node* child_[kNodeMax + 1];
  InteriorNode* parent_;
};

class BorderNode : public Node {
 public:
  BorderNode() : Node(kNodeIsBorder) {
  };
  uint8_t n_removed_;
  uint8_t key_len_[kNodeMax];
  uint64_t permutation_;
  uint64_t key_slice_[kNodeMax];
  // link_or_value lv[kNodeMax];
  void* link_or_value_[kNodeMax];
  BorderNode* prev_;
  BorderNode* next_;
  InteriorNode* parent_;
  //keysuffix_t key_suffixes_;
};

class Masstree {
 public:
  Masstree();
  ~Masstree();
 private:
  Node* root_;
};
}  // namespace masstree;

#endif  // MASSTREE_H_
