#pragma once
#include "ucg_editor_model.hpp"
#include "storage_hooks.hpp"
#include "release_iso_map.hpp"
#include "release_paths.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>
namespace EditorFiles {
struct Entry {
  std::string name;
  bool directory = false;
};
inline bool directory(const std::string &p) {
  struct stat s{};
  return stat(p.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
}
inline bool exists(const std::string &p) {
  struct stat s{};
  return stat(p.c_str(), &s) == 0 || ucg_storage::virtualExists(p.c_str());
}
inline std::string base(const std::string &p) {
  auto n = p.find_last_of("/\\");
  return n == std::string::npos ? "" : p.substr(0, n);
}
inline std::string name(const std::string &p) {
  auto n = p.find_last_of("/\\");
  return n == std::string::npos ? p : p.substr(n + 1);
}
inline bool extension(const std::string &p, const char *e) {
  const size_t size = std::strlen(e);
  if (p.size() < size)
    return false;
  // FAT may enumerate legal 8.3 filenames in uppercase. Fold only the ASCII
  // suffix, retaining the exact filename for subsequent opens and saves.
  auto lower = [](unsigned char c) {
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
  };
  for (size_t i = 0; i < size; ++i)
    if (lower(p[p.size() - size + i]) != lower(e[i]))
      return false;
  return true;
}
inline bool make(const std::string &p) {
  if (directory(p))
    return true;
  auto parent = base(p);
  return (parent.empty() || parent == p || directory(parent) || make(parent)) &&
         mkdir(p.c_str(), 0777) == 0;
}
inline bool join(const std::string &root, std::string relative,
                 std::string &out) {
  if (relative.find('\0') != std::string::npos ||
      relative.find(':') != std::string::npos)
    return false;
  std::replace(relative.begin(), relative.end(), '\\', '/');
  if (!relative.empty() && relative[0] == '/')
    return false;
  std::vector<std::string> pieces;
  size_t at = 0;
  while (at <= relative.size()) {
    auto end = relative.find('/', at);
    if (end == std::string::npos)
      end = relative.size();
    auto p = relative.substr(at, end - at);
    if (p == "..") {
      if (pieces.empty())
        return false;
      pieces.pop_back();
    } else if (!p.empty() && p != ".")
      pieces.push_back(p);
    at = end + 1;
  }
  out = root;
  for (const auto &p : pieces)
    out += "/" + p;
  return true;
}
inline bool list(const std::string &root, const std::string &relative,
                 std::vector<Entry> &out, std::string &error) {
  std::string path;
  if (!join(root, relative, path)) {
    error = "Invalid folder";
    return false;
  }
  std::vector<Entry> next;
  if (!relative.empty())
    next.push_back({"..", true});
  if (ReleasePaths::disc(path) && path.rfind(ReleasePaths::assetDirectory(path.c_str()),0)==0 &&
      ucg_storage::virtualDirectory(path.c_str())) {
    // The physical ISO is flat. Derive packaged logical children from the
    // same complete map used by descriptor opens, without probing ISO folders.
    const auto discRoot=ReleasePaths::assetDirectory(path.c_str());
    std::string prefix=path.substr(discRoot.size());
    if(prefix.empty() || prefix.back()!='/')prefix+='/';
    bool found=false;
    for(const auto&entry:ReleaseIsoMap::entries) {
      if(entry.logical.substr(0,prefix.size())!=prefix)continue;
      found=true;
      const auto tail=entry.logical.substr(prefix.size());
      const auto slash=tail.find('/');
      const bool folder=slash!=std::string_view::npos;
      std::string name(tail.substr(0,slash));
      if(!folder && extension(name,".ucb"))name.resize(name.size()-4);
      const auto previous=std::find_if(next.begin(),next.end(),[&](const Entry&value){return value.name==name;});
      if(previous!=next.end()){previous->directory=previous->directory||folder;continue;}
      if(next.size()>=2048){error="Folder has more than 2048 entries";return false;}
      next.push_back({std::move(name),folder});
    }
    if(!found){error="Cannot open folder: "+path;return false;}
  } else {
   auto *d = opendir(path.c_str());
   if (!d) {
     error = "Cannot open folder: " + path;
     return false;
   }
   while (auto *e = readdir(d)) {
    std::string n = e->d_name;
    if (n == "." || n == "..")
      continue;
    // Builtin ExampleWorld is stored virtually, then copied through fopen to
    // ordinary editable files. User directories retain their exact filenames.
    if (ucg_storage::virtualDirectory(path.c_str()) && n.size()>4 &&
        extension(n,".ucb")) n.resize(n.size()-4);
    if (next.size() >= 2048) {
      closedir(d);
      error = "Folder has more than 2048 entries";
      return false;
    }
    next.push_back({n, directory(path + "/" + n)});
   }
   closedir(d);
  }
  std::stable_sort(
      next.begin(), next.end(), [](const Entry &a, const Entry &b) {
        if (a.name == ".." || b.name == "..")
          return a.name == ".." && b.name != "..";
        return a.directory != b.directory ? a.directory > b.directory
                                          : a.name < b.name;
      });
  out = std::move(next);
  error.clear();
  return true;
}
inline bool copy(const std::string &from, const std::string &to) {
  FILE *in = std::fopen(from.c_str(), "rb");
  if (!in)
    return false;
  FILE *out = std::fopen(to.c_str(), "wb");
  if (!out) {
    std::fclose(in);
    return false;
  }
  char buffer[8192];
  bool ok = true;
  for (;;) {
    const size_t n = std::fread(buffer, 1, sizeof buffer, in);
    if (n && std::fwrite(buffer, 1, n, out) != n) {
      ok = false;
      break;
    }
    if (n < sizeof buffer) {
      ok = std::ferror(in) == 0;
      break;
    }
  }
  if (std::fflush(out))
    ok = false;
  if (std::fclose(out))
    ok = false;
  if (std::fclose(in))
    ok = false;
  return ok;
}
inline bool installExample(const std::string &source, const std::string &root,
                           std::string &error) {
  if (directory(root + "/ExampleWorld"))
    return true;
  if (!make(root)) {
    error = "Cannot create custom_levels";
    return false;
  }
  std::string staged;
  for (unsigned i = 0; i < 1000; ++i) {
    staged = root + "/ExampleWorld.install-" + std::to_string(i);
    if (!exists(staged))
      break;
    if (i == 999) {
      error = "No free install folder";
      return false;
    }
  }
  if (!make(staged)) {
    error = "Cannot stage Example World";
    return false;
  }
  std::vector<Entry> files;
  if (!list(source, "", files, error))
    return false;
  for (const auto &f : files)
    if (!f.directory && !copy(source + "/" + f.name, staged + "/" + f.name)) {
      error = "Example World copy failed: " + f.name;
      return false;
    }
  if (std::rename(staged.c_str(), (root + "/ExampleWorld").c_str())) {
    error = "Cannot finish Example World installation";
    return false;
  }
  return true;
}
struct World {
  std::string directory, name, author, description, icon;
  std::vector<std::string> levels;
  bool named=false;
};
inline bool worlds(const std::string &root, std::vector<World> &result,
                   std::string &error) {
  std::vector<Entry> folders;
  if (!list(root, "", folders, error))
    return false;
  std::vector<World> next;
  for (const auto &f : folders) {
    if (!f.directory || f.name == ".." || f.name.rfind("ExampleWorld.install-",0)==0 ||
        !exists(root + "/" + f.name + "/info.json"))
      continue;
    ucg::editor::Document info;
    std::string local;
    if (!ucg::editor::load_file(root + "/" + f.name + "/info.json", info,
                                local)) {
      error = f.name + "/info.json: " + local;
      return false;
    }
    World w;
    w.directory = f.name;
    w.name = "Untitled";
    w.description="No description.";
    for (auto field : {"name", "author", "desc"})
      if (const auto *v = info.root.find(field))
        if (v->kind == ucg::editor::Value::Kind::String) {
          if (!std::strcmp(field, "name")){w.name = v->text;w.named=true;}
          else if (!std::strcmp(field, "author"))
            w.author = v->text;
          else
            w.description = v->text;
        }
    w.icon = exists(root + "/" + f.name + "/icon.jpg")   ? "icon.jpg"
             : exists(root + "/" + f.name + "/icon.png") ? "icon.png"
                                                         : "";
    std::vector<Entry> files;
    if (!list(root, f.name, files, local)) {
      error = local;
      return false;
    }
    for (const auto &file : files)
      if (!file.directory && extension(file.name, ".json") &&
          file.name != "info.json") {
        ucg::editor::Document level;
        if (!ucg::editor::load_file(root + "/" + f.name + "/" + file.name,
                                    level, local)) {
          error = f.name + "/" + file.name + ": " + local;
          return false;
        }
        if (level.root.find("id") && level.root.find("world"))
          w.levels.push_back(file.name);
      }
    next.push_back(std::move(w));
  }
  result = std::move(next);
  return true;
}
} // namespace EditorFiles
