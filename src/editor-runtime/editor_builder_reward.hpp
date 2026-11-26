#pragma once
#include "locale.hpp"
#include <functional>
#include <string>
namespace EditorBuilder {
inline constexpr uint32_t Magic = 0x45424355;
inline constexpr const char *FileName = "editor-builder.cfg";
// Preserve the released/previous direct-owned marker: value1 stays owned.
enum class RewardState : uint32_t { None = 0, Owned = 1, Pending = 2 };
inline bool validState(uint32_t value) { return value <= 2; }
using File = PreferenceFile<Magic, validState>;
class Store {
public:
  std::function<bool(uint32_t, std::string &)> save;
  bool restore(uint32_t value, std::string &e) {
    e.clear();
    if (!validState(value)) {
      e = "Invalid Builder reward state";
      return false;
    }
    status = static_cast<RewardState>(value);
    return true;
  }
  bool queue(bool &added, std::string &e) {
    added = false;
    e.clear();
    if (status != RewardState::None)
      return true;
    if (!commit(RewardState::Pending, e))
      return false;
    added = true;
    return true;
  }
  bool queue(std::string &e) {
    bool added = false;
    return queue(added, e);
  }
  bool markOpened(std::string &e) {
    e.clear();
    if (owned())
      return true;
    if (!pending()) {
      e = "Builder present is not pending";
      return false;
    }
    return commit(RewardState::Owned, e);
  }
  bool pending() const { return status == RewardState::Pending; }
  bool owned() const { return status == RewardState::Owned; }
  uint32_t value() const { return static_cast<uint32_t>(status); }
  RewardState state() const { return status; }

private:
  RewardState status = RewardState::None;
  bool commit(RewardState next, std::string &e) {
    if (!save) {
      e = "Builder persistence callback is unavailable";
      return false;
    }
    if (!save(static_cast<uint32_t>(next), e)) {
      if (e.empty())
        e = "Cannot save Builder reward";
      return false;
    }
    status = next;
    e.clear();
    return true;
  }
};
} // namespace EditorBuilder
