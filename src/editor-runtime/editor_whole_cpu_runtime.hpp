#pragma once
// Included INSIDE EditorRender after allocateCpuCache; all storage below is
// charged by reservedSampleBytes() through wholeCpuAccountedBytes().
inline EditorWholeCpu::Reader wholeCpuReader;
inline EditorWholeCpu::Cache wholeCpuCache;
inline FILE *wholeCpuFile = nullptr;
inline bool wholeCpuAttempted = false;
inline const char *wholeCpuLastError = nullptr;
inline std::array<std::string,2> wholeCpuSourceNames;
inline bool wholeCpuSourceBacked(const std::string&file){
  for(const auto&name:wholeCpuSourceNames)if(!name.empty()&&file==name)return true;
  return false;
}
// Called only with a live lease for this explicit original asset ID.
inline bool bindWholeCpuSource(EditorWholeCpu::Asset asset,const std::string&file){
  if(!EditorWholeCpu::known(asset)||file.empty())return false;
  auto&name=wholeCpuSourceNames[uint32_t(asset)];if(name==file)return false;name=file;return true;
}
inline size_t wholeCpuAccountedBytes() {
  size_t names=sizeof wholeCpuSourceNames;
  for(const auto&name:wholeCpuSourceNames)if(name.capacity()>15)names+=name.capacity()+1;
  return names+wholeCpuCache.accountedBytes() + sizeof(wholeCpuReader) +
         sizeof(wholeCpuFile) + sizeof(wholeCpuAttempted) +
         sizeof(wholeCpuLastError);
}
inline bool wholeCpuRead(void *context, uint32_t offset, void *out,
                         size_t bytes) {
  auto *file = static_cast<FILE *>(context);
  return file && std::fseek(file, long(offset), SEEK_SET) == 0 &&
         std::fread(out, 1, bytes, file) == bytes;
}
inline bool admitWholeCpu(void *, size_t additional, size_t largest) {
  for (unsigned attempt = 0; attempt <= PageCacheCount; ++attempt) {
    if (uint64_t(cacheTotalBytes()) + reservedSampleBytes() + additional <=
            CacheBudget &&
        EditorMemory::probe(additional, largest, 0))
      return true;
    Page *oldest = oldestPage();
    if (!oldest)
      return false;
    flushBatch();
    discardPage(*oldest);
  }
  return false;
}
inline void *allocateWholeCpu(void *, size_t bytes) {
  return allocateCpuCache(bytes);
}
inline void freeWholeCpu(void *, void *memory) { std::free(memory); }
inline bool openWholeCpu() {
  if (wholeCpuAttempted)
    return wholeCpuReader.ready();
  wholeCpuAttempted = true;
  const auto name =
      companion(executable.c_str(), "data/world/editor-whole-cpu.bin");
  wholeCpuFile = std::fopen(name.c_str(), "rb");
  if (!wholeCpuFile) {
    wholeCpuLastError = "Immutable CPU image bank unavailable";
    return false;
  }
  // No hidden large stdio input buffer; decode uses a bounded4KiB stack block.
  bool ok = std::setvbuf(wholeCpuFile, nullptr, _IONBF, 0) == 0 &&
            std::fseek(wholeCpuFile, 0, SEEK_END) == 0;
  long length = ok ? std::ftell(wholeCpuFile) : -1;
  ok = ok && length >= 0 && uint64_t(length) <= UINT32_MAX &&
       wholeCpuReader.open(wholeCpuRead, wholeCpuFile, uint32_t(length),
                           wholeCpuLastError);
  if (!ok) {
    if (!wholeCpuLastError)
      wholeCpuLastError = "Immutable CPU image bank open failed";
    std::fclose(wholeCpuFile);
    wholeCpuFile = nullptr;
  }
  return ok;
}
inline EditorWholeCpu::Cache::Lease
acquireWholeCpu(EditorWholeCpu::Asset asset) {
  if (!EditorWholeCpu::known(asset) || !EditorArt::workspace || !openWholeCpu())
    return {};
  return wholeCpuCache.acquire(
      asset, wholeCpuReader,
      {admitWholeCpu, allocateWholeCpu, freeWholeCpu, nullptr},
      EditorArt::workspace, EditorWholeCpu::WorkspaceBytes, wholeCpuLastError);
}
inline void releaseWholeCpu() {
  for(auto&name:wholeCpuSourceNames)std::string().swap(name);
  wholeCpuCache.clear();
  wholeCpuReader.close();
  if (wholeCpuFile)
    std::fclose(wholeCpuFile);
  wholeCpuFile = nullptr;
  wholeCpuAttempted = false;
  wholeCpuLastError = nullptr;
}
