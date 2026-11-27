#pragma once
// One UCGP00054GAME entry, two protected files. Not a power-loss guarantee:
// both generations share firmware-owned PARAM.SFO. Never use CREATE to repair.
#include "native_state_archive072.hpp"
#ifdef __PSP__
#include <pspdisplay.h>
#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <psputility.h>
#include <strings.h>
#endif
namespace NativeSave074 {
enum class LoadResult { Loaded, Missing, Invalid, IOError };
constexpr uint32_t NoData = 0x80110327u, NoFile = 0x80110329u,
                   Broken = 0x80110326u;
enum {
  Files = 12,
  Create = 13,
  Read = 15,
  Write = 17,
  Space = 22,
  Single = 0,
  Start = 1,
  Relay = 2,
  End = 3
};
constexpr uint32_t WatchdogUs = 30000000u;
// Public, nonsecret format identifier. Changing it makes existing native saves
// unreadable.
inline constexpr unsigned char SavedataKey[16] = {
    0x55, 0x43, 0x47, 0x50, 0x30, 0x30, 0x30, 0x35,
    0x34, 0x53, 0x41, 0x56, 0x45, 0x30, 0x37, 0x34};
struct Request {
  int mode = 0, multi = 0, slot = 0, result = INT_MAX, abortStatus = INT_MAX;
  unsigned char *data = nullptr;
  size_t capacity = 0, length = 0;
  unsigned mask = 0;
  bool malformed = false;
  size_t sizes[2]{};
  unsigned writeMask = 0;
  int neededKB = -1, overwriteKB = -1;
  const unsigned char *icon = nullptr;
  size_t iconSize = 0;
};
// A transport must retain request pointers until status()==NONE. Test
// transports implement this same lifecycle; no FILE/utility behavior is mocked
// in Archive.
class Transport {
public:
  bool busy = false, tainted = false;
  virtual ~Transport() = default;
  virtual int init(Request &) = 0;
  virtual int status() = 0;
  virtual void update() = 0;
  virtual int shutdown() = 0;
  virtual uint32_t now() = 0;
  virtual void frame() = 0;
  virtual bool present(bool &, int &) = 0;
};
#ifdef __PSP__
class PSPTransport final : public Transport {
  SceUtilitySavedataParam param_{};
  SceUtilitySavedataFileListInfo list_{};
  SceUtilitySavedataFileListEntry secure_[3]{}, normal_[1]{}, system_[5]{};
  PspUtilitySavedataSizeInfo space_{};
  PspUtilitySavedataSizeEntry sizes_[2]{};
  Request *request_ = nullptr;
  static const char *name(int n) { return n ? "DATA1.BIN" : "DATA0.BIN"; }
  void result() {
    if (!request_)
      return;
    auto &r = *request_;
    r.result = param_.base.result;
    r.abortStatus = param_.abortStatus;
    r.length = param_.dataSize;
    if (r.mode == Space) {
      r.neededKB = space_.neededKB;
      r.overwriteKB = space_.overwriteKB;
    }
    if (r.mode == Files && r.result == 0) {
      r.mask = 0;
      r.malformed =
          list_.resultNumSecureEntries > 2 || list_.resultNumNormalEntries != 0;
      for (unsigned i = 0; i < list_.resultNumSecureEntries && i < 3; ++i) {
        unsigned bit = 0;
        if (!std::strncmp(secure_[i].name, "DATA0.BIN", 16))
          bit = 1;
        else if (!std::strncmp(secure_[i].name, "DATA1.BIN", 16))
          bit = 2;
        if (!bit || (r.mask & bit))
          r.malformed = true;
        r.mask |= bit;
      }
    }
    request_ = nullptr;
  }

public:
  int init(Request &r) override {
    // All firmware-facing structs are pinned in this process-lifetime adapter.
    if (tainted || request_)
      return -EBUSY;
    std::memset(&param_, 0, sizeof param_);
    param_.base.size = sizeof param_;
    param_.base.language = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
    param_.base.buttonSwap = PSP_UTILITY_ACCEPT_CROSS;
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,
                                &param_.base.language);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,
                                &param_.base.buttonSwap);
    param_.base.graphicsThread = 17;
    param_.base.accessThread = 19;
    param_.base.fontThread = 18;
    param_.base.soundThread = 16;
    param_.base.result = INT_MAX;
    param_.abortStatus = INT_MAX;
    param_.mode = static_cast<PspUtilitySavedataMode>(r.mode);
    param_.multiStatus = r.multi;
    param_.secureVersion = 0;
    std::strcpy(param_.gameName, "UCGP00054");
    std::strcpy(param_.saveName, "GAME");
    std::memcpy(param_.key, SavedataKey, 16);
    if (r.mode == Read || r.mode == Write || r.mode == Create)
      std::strcpy(param_.fileName, name(r.slot));
    param_.dataBuf = r.data;
    param_.dataBufSize = r.capacity;
    param_.dataSize = r.length;
    if (r.mode == Files) {
      std::memset(&list_, 0, sizeof list_);
      std::memset(secure_, 0, sizeof secure_);
      std::memset(normal_, 0, sizeof normal_);
      std::memset(system_, 0, sizeof system_);
      list_.maxSecureEntries = 3;
      list_.maxNormalEntries = 1;
      list_.maxSystemEntries = 5;
      list_.secureEntries = secure_;
      list_.normalEntries = normal_;
      list_.systemEntries = system_;
      param_.fileList = &list_;
    }
    if (r.mode == Space) {
      std::memset(&space_, 0, sizeof space_);
      std::memset(sizes_, 0, sizeof sizes_);
      unsigned n = 0;
      for (int i = 0; i < 2; ++i)
        if (r.writeMask & (1u << i)) {
          sizes_[n].size = r.sizes[i];
          std::strcpy(sizes_[n].name, name(i));
          ++n;
        }
      space_.numSecureEntries = n;
      space_.secureEntries = sizes_;
      param_.sizeInfo = &space_;
    }
    if (r.mode == Create) {
      std::strcpy(param_.sfoParam.title, "Uncanny Cat Golf");
      std::strcpy(param_.sfoParam.savedataTitle, "All profiles");
      std::strcpy(
          param_.sfoParam.detail,
          "Shared progress, profiles and settings. Native save format 1.");
      param_.sfoParam.parentalLevel = 1;
    }
    if (r.mode == Create || (r.mode == Space && r.writeMask == 3)) {
      param_.icon0FileData.buf = const_cast<unsigned char *>(r.icon);
      param_.icon0FileData.bufSize = r.iconSize;
      param_.icon0FileData.size = r.iconSize;
    }
    request_ = &r;
    const int v = sceUtilitySavedataInitStart(&param_);
    if (v != 0)
      request_ = nullptr;
    return v;
  }
  int status() override {
    const int s = sceUtilitySavedataGetStatus();
    if (s == 0)
      result();
    return s;
  }
  void update() override { sceUtilitySavedataUpdate(1); }
  int shutdown() override { return sceUtilitySavedataShutdownStart(); }
  uint32_t now() override { return sceKernelGetSystemTimeLow(); }
  void frame() override { sceDisplayWaitVblankStart(); }
  bool present(bool &present, int &error) override {
    present = false;
    const int fd = sceIoDopen("ms0:/PSP/SAVEDATA");
    if (fd < 0) {
      error = fd;
      return false;
    }
    SceIoDirent entry{};
    int r = 0;
    while ((r = sceIoDread(fd, &entry)) > 0) {
      if (!strcasecmp(entry.d_name, "UCGP00054GAME"))
        present = true;
      std::memset(&entry, 0, sizeof entry);
    }
    const int close = sceIoDclose(fd);
    if (r != 0 || close != 0) {
      error = r < 0 ? r : close;
      return false;
    }
    error = 0;
    return true;
  }
};
inline Transport &defaultTransport() {
  static PSPTransport t;
  return t;
}
static_assert(sizeof(SceUtilitySavedataParam) == 1536,
              "savedata ABI requires PSP_FW_VERSION >= 200");
#else
class UnavailableTransport final : public Transport {
public:
  int init(Request &) override { return -ENOSYS; }
  int status() override { return 0; }
  void update() override {}
  int shutdown() override { return -ENOSYS; }
  uint32_t now() override { return 0; }
  void frame() override {}
  bool present(bool &, int &e) override {
    e = -ENOSYS;
    return false;
  }
};
inline Transport &defaultTransport() {
  static UnavailableTransport t;
  return t;
}
#endif
struct Diagnostic {
  int beforeStatus=INT_MAX,initResult=INT_MAX,lastStatus=INT_MAX;
  int mode=0,multi=0;
};
class Controller {
  enum Peer { MissingPeer, PayloadPeer, BrokenPeer };
  struct Pair {
    NativeState072::Bytes bytes[2];
    uint64_t gen[2]{};
    Peer kind[2]{MissingPeer, MissingPeer};
    bool valid[2]{};
    unsigned mask = 0;
  };
  struct State {
    Request request;
    Diagnostic diagnostic;
    NativeState072::Bytes icon, encoded;
    Pair known, read;
    bool ready = false, absent = false, poisoned = false, active = false,
         shutdownRequested = false;
    bool pending = false, pendingFresh = false;
    unsigned pendingMask = 0;
    uint64_t generation = 0;
    int error = 0;
    void (*pump)(void *) = nullptr;
    void *context = nullptr;
  };
  Transport &t_;
  State *s_ = nullptr;
  int initialError_ = -ENOMEM;
  bool fail(int e) {
    if (s_)
      s_->error = e ? e : -EIO;
    return false;
  }
  bool poison(int e) {
    s_->poisoned = true;
    t_.tainted = true;
    return fail(e);
  }
  bool begin() {
    if (!s_)
      return false;
    if (s_->poisoned || t_.tainted)
      return fail(-EBUSY);
    if (t_.busy)
      return fail(-EBUSY);
    t_.busy = true;
    s_->error = 0;
    return true;
  }
  struct Unlock {
    Transport &t;
    ~Unlock() { t.busy = false; }
  };
  void setup(int mode, int multi, int slot = 0) {
    s_->request = Request{};
    s_->request.mode = mode;
    s_->request.multi = multi;
    s_->request.slot = slot;
  }
  bool run() {
    if (s_->active || s_->poisoned || t_.tainted)
      return fail(-EBUSY);
    s_->diagnostic=Diagnostic{};
    s_->diagnostic.mode=s_->request.mode;
    s_->diagnostic.multi=s_->request.multi;
    // Before our first accepted InitStart, GetStatus may report an error or a
    // prior utility state. It does not establish ownership. The checked SDK
    // InitStart arbitrates admission; our own busy/poison guards remain above.
    s_->diagnostic.beforeStatus=t_.status();
    const int init = t_.init(s_->request);
    s_->diagnostic.initResult=init;
    if (init != 0)
      return fail(init);
    s_->active = true;
    s_->shutdownRequested = false;
    const uint32_t start = t_.now();
    while (s_->active) {
      if (s_->pump)
        s_->pump(s_->context);
      const int status = t_.status();
      s_->diagnostic.lastStatus=status;
      if (status == 2)
        t_.update();
      else if (status == 3 && !s_->shutdownRequested) {
        s_->shutdownRequested = true;
        const int e = t_.shutdown();
        if (e != 0)
          return poison(e);
      } else if (status == 0) {
        s_->active = false;
        if (!s_->shutdownRequested)
          return fail(-EIO);
      } else if (status < 0 || status > 4)
        return poison(status < 0 ? status : -EIO);
      if (s_->active) {
        t_.frame();
        if (uint32_t(t_.now() - start) > WatchdogUs)
          return poison(-ETIMEDOUT);
      }
    }
    return true;
  }
  bool ok() {
    return run() && (s_->request.result == 0 || fail(s_->request.result));
  }
  bool rawAbsent() {
    bool present = false;
    int e = 0;
    if (!t_.present(present, e))
      return fail(e);
    return !present || fail(-EEXIST);
  }
  bool files(int multi, unsigned &mask) {
    setup(Files, multi);
    if (!ok())
      return false;
    if (s_->request.malformed)
      return fail(-EINVAL);
    mask = s_->request.mask;
    return true;
  }
  static void reset(Pair &p) {
    for (int i = 0; i < 2; ++i) {
      p.bytes[i] = NativeState072::Bytes();
      p.gen[i] = 0;
      p.kind[i] = MissingPeer;
      p.valid[i] = false;
    }
    p.mask = 0;
  }
  bool readSlot(Pair &p, int slot, int multi) {
    auto &b = p.bytes[slot];
    if (!b.resize(NativeState072::MaxArchiveBytes))
      return fail(-errno);
    setup(Read, multi, slot);
    auto &r = s_->request;
    r.data = b.data();
    r.capacity = b.size();
    r.length = 0;
    if (!run())
      return false;
    if (uint32_t(r.result) == Broken || uint32_t(r.result) == NoFile) {
      p.kind[slot] = BrokenPeer;
      b = NativeState072::Bytes();
      return true;
    }
    if (r.result != 0)
      return fail(r.result);
    p.kind[slot] = PayloadPeer;
    // Firmware must never make a larger returned size an unchecked buffer read.
    if (r.length > r.capacity)
      return fail(-EFBIG);
    if (!b.resize(r.length))
      return fail(-errno);
    NativeState072::Archive decoded;
    if (NativeState072::decode(b, decoded, p.gen[slot]))
      p.valid[slot] = true;
    else if (errno == ENOMEM)
      return fail(-ENOMEM);
    return true;
  }
  // Read before WRITE in the same monitored START chain. Native-auth failures
  // never authorize repair. A codec-invalid payload can be compared
  // byte-for-byte.
  bool pair(Pair &p, bool end) {
    reset(p);
    if (!files(Start, p.mask))
      return false;
    if (!p.mask)
      return fail(-EINVAL);
    for (int i = 0; i < 2; ++i)
      if (p.mask & (1u << i)) {
        const bool last = i == 1 || !(p.mask & 2);
        if (!readSlot(p, i, end && last ? End : Relay))
          return false;
        if (p.kind[i] == BrokenPeer && !last) {
          // The failed call ends this logical transaction: independently
          // restart to recover the other readable slot, never continue to a
          // write after failure.
          unsigned mask = 0;
          if (!files(Start, mask) || mask != p.mask)
            return fail(-ESTALE);
          if (!readSlot(p, 1, end ? End : Relay))
            return false;
          break;
        }
      }
    if (p.valid[0] && p.valid[1] && p.gen[0] == p.gen[1] &&
        !p.bytes[0].equals(p.bytes[1]))
      return fail(-EINVAL);
    return true;
  }
  static int newest(const Pair &p) {
    if (!p.valid[0])
      return p.valid[1] ? 1 : -1;
    if (!p.valid[1])
      return 0;
    return p.gen[1] > p.gen[0] ? 1 : 0;
  }
  static bool equal(const Pair &a, const Pair &b) {
    if (a.mask != b.mask)
      return false;
    for (int i = 0; i < 2; ++i)
      if (a.kind[i] != b.kind[i] || a.kind[i] == BrokenPeer ||
          !a.bytes[i].equals(b.bytes[i]))
        return false;
    return true;
  }
  static void adopt(Pair &to, Pair &from) {
    reset(to);
    to.mask = from.mask;
    for (int i = 0; i < 2; ++i) {
      to.bytes[i].swap(from.bytes[i]);
      to.gen[i] = from.gen[i];
      to.kind[i] = from.kind[i];
      to.valid[i] = from.valid[i];
    }
  }
  bool space(bool fresh, int slot) {
    setup(Space, Relay);
    auto &r = s_->request;
    r.writeMask = fresh ? 3 : 1u << slot;
    r.sizes[0] = s_->encoded.size();
    r.sizes[1] = s_->encoded.size();
    r.icon = s_->icon.data();
    r.iconSize = s_->icon.size();
    if (!run())
      return false;
    if (fresh) {
      if (uint32_t(r.result) != NoData)
        return fail(r.result ? r.result : -EEXIST);
      return r.neededKB == 0 || fail(-ENOSPC);
    }
    if (r.result != 0)
      return fail(r.result);
    return r.overwriteKB == 0 || fail(-ENOSPC);
  }
  bool write(int slot, bool create, int multi) {
    setup(create ? Create : Write, multi, slot);
    auto &r = s_->request;
    r.data = s_->encoded.data();
    r.capacity = r.length = s_->encoded.size();
    if (create) {
      r.icon = s_->icon.data();
      r.iconSize = s_->icon.size();
    }
    // A failure after this point leaves the physical result indeterminate.
    // Retry requires exact original/attempted-byte revalidation in recover().
    s_->ready = false;
    s_->pending = true;
    return ok();
  }
  bool verify(bool fresh, int target) {
    if (!pair(s_->read, true))
      return false;
    for (int i = 0; i < 2; ++i) {
      if (fresh || i == target) {
        if (!s_->read.valid[i] || !s_->read.bytes[i].equals(s_->encoded))
          return fail(-EIO);
      } else if (s_->read.kind[i] != s_->known.kind[i] ||
                 !s_->read.bytes[i].equals(s_->known.bytes[i]))
        return fail(-EIO);
    }
    adopt(s_->known, s_->read);
    s_->generation = s_->known.gen[target];
    s_->ready = true;
    s_->absent = false;
    s_->pending = false;
    return true;
  }
  bool recover() {
    // Do not use public load here: it could silently accept an unrelated card.
    // The previous attempted bytes remain pinned in encoded until this
    // succeeds.
    if (!s_->pending)
      return fail(-ESTALE);
    if (!pair(s_->read, true)) {
      if (s_->pendingFresh && uint32_t(s_->error) == NoData &&
          s_->read.mask == 0 && s_->request.mode == Files &&
          s_->request.multi == Start && rawAbsent()) {
        reset(s_->known);
        s_->generation = 0;
        s_->ready = true;
        s_->absent = true;
        s_->pending = false;
        s_->error = 0;
        return true;
      }
      return false;
    }
    for (int i = 0; i < 2; ++i) {
      if (s_->read.kind[i] == BrokenPeer)
        return fail(-ESTALE);
      const bool original = s_->read.kind[i] == s_->known.kind[i] &&
                            s_->read.bytes[i].equals(s_->known.bytes[i]);
      const bool attempted = (s_->pendingMask & (1u << i)) &&
                             s_->read.valid[i] &&
                             s_->read.bytes[i].equals(s_->encoded);
      if (!original && !attempted)
        return fail(-ESTALE);
    }
    const int n = newest(s_->read);
    if (n < 0)
      return fail(-ESTALE);
    adopt(s_->known, s_->read);
    s_->generation = s_->known.gen[n];
    s_->ready = true;
    s_->absent = false;
    s_->pending = false;
    s_->error = 0;
    return true;
  }

public:
  Controller() : Controller(defaultTransport()) {}
  explicit Controller(Transport &t) : t_(t) {
    void *p = std::malloc(sizeof(State));
    if (p)
      s_ = new (p) State;
  }
  Controller(const Controller &) = delete;
  Controller &operator=(const Controller &) = delete;
  ~Controller() {
    // Firmware may still own pointers on timeout/shutdown failure. Deliberate
    // bounded process-lifetime retention beats use-after-free, including icon.
    if (s_ && !s_->poisoned) {
      s_->~State();
      std::free(s_);
    }
  }
  void configure(const unsigned char *icon, size_t size, void (*pump)(void *),
                 void *context) {
    if (!s_)
      return;
    if (s_->active || s_->poisoned || t_.busy) {
      fail(-EBUSY);
      return;
    }
    if (!s_->icon.assign(icon, size)) {
      fail(-errno);
      return;
    }
    s_->pump = pump;
    s_->context = context;
  }
  LoadResult load(NativeState072::Archive &out) {
    if (!begin())
      return LoadResult::IOError;
    Unlock unlock{t_};
    s_->ready = false;
    s_->absent = false;
    s_->pending = false;
    if (!pair(s_->read, true)) {
      if (uint32_t(s_->error) == NoData && s_->read.mask == 0 &&
          s_->request.mode == Files && s_->request.multi == Start) {
        if (!rawAbsent())
          return s_->error == -EEXIST ? LoadResult::Invalid
                                      : LoadResult::IOError;
        reset(s_->known);
        s_->generation = 0;
        s_->ready = true;
        s_->absent = true;
        s_->error = 0;
        return LoadResult::Missing;
      }
      return s_->error == -EINVAL || s_->error == -EFBIG ||
                     uint32_t(s_->error) == Broken ||
                     uint32_t(s_->error) == NoFile
                 ? LoadResult::Invalid
                 : LoadResult::IOError;
    }
    const int n = newest(s_->read);
    if (n < 0) {
      fail(-EINVAL);
      return LoadResult::Invalid;
    }
    uint64_t gen = 0;
    if (!NativeState072::decode(s_->read.bytes[n], out, gen)) {
      fail(-errno);
      return errno == ENOMEM ? LoadResult::IOError : LoadResult::Invalid;
    }
    adopt(s_->known, s_->read);
    s_->generation = gen;
    s_->ready = true;
    s_->error = 0;
    return LoadResult::Loaded;
  }
  bool save(const NativeState072::Archive &archive) {
    if (!begin())
      return false;
    Unlock unlock{t_};
    if (!s_->ready && !recover())
      return false;
    if (s_->generation == UINT64_MAX)
      return fail(-EOVERFLOW);
    const uint64_t generation = s_->absent ? 1 : s_->generation + 1;
    if (!NativeState072::encode(archive, generation, s_->encoded))
      return fail(-errno);
    if (s_->absent) {
      if (!rawAbsent())
        return false;
      setup(Files, Start);
      if (!run())
        return false;
      if (uint32_t(s_->request.result) != NoData)
        return fail(s_->request.result ? s_->request.result : -EEXIST);
      if (!space(true, 0) || !rawAbsent())
        return false;
      s_->pendingFresh = true;
      s_->pendingMask = 3;
      if (!write(0, true, Relay) || !write(1, false, End))
        return false;
      return verify(true, 0);
    }
    if (!pair(s_->read, false))
      return false;
    if (!equal(s_->known, s_->read))
      return fail(-ESTALE);
    const int current = newest(s_->read);
    if (current < 0)
      return fail(-EINVAL);
    const int target = 1 - current;
    if (!space(false, target))
      return false;
    s_->pendingFresh = false;
    s_->pendingMask = 1u << target;
    if (!write(target, false, End))
      return false;
    return verify(false, target);
  }
  Diagnostic diagnostic() const { return s_?s_->diagnostic:Diagnostic{}; }
  int error() const { return s_ ? s_->error : initialError_; }
  bool poisoned() const { return t_.tainted || (s_ && s_->poisoned); }
  uint64_t generation() const { return s_ ? s_->generation : 0; }
  // Optional per-frame drain after timeout. Call after finishing game drawing;
  // it never restarts an operation, unpoisons, waits a frame or invokes pump.
  void drain() {
    if (!s_ || !s_->poisoned || !s_->active)
      return;
    const int status = t_.status();
    if (status == 2)
      t_.update();
    else if (status == 3 && !s_->shutdownRequested) {
      s_->shutdownRequested = true;
      t_.shutdown();
    } else if (status == 0)
      s_->active = false;
  }
};
} // namespace NativeSave074
