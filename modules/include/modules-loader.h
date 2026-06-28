/*!==========================================================================
* \file
* - Program:       palmira
* - File:          modules-loader.h
* - Created:       06/26/2026
* - Author:        Vitaly Bulganin
* - Description:
* - Comments:
*
-----------------------------------------------------------------------------
*
* - History:
*
===========================================================================*/
#pragma once
//-------------------------------------------------------------------------//
#ifndef __MODULES_LOADER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __MODULES_LOADER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <dlfcn.h>
//-------------------------------------------------------------------------//
class modules_loader final {
  //!< Keeps a path of plugins.
  const std::string plugins_path;

  //!< Keeps a list of loaded plugins.
  mutable std::unordered_map<std::string, void *> plugins;

public:
  /**
   * Constructor.
   * @param path [in] - A path of keeping plugins.
   */
  explicit modules_loader(std::string path) : plugins_path(std::move(path)) {
    auto push_module = [&](const std::string &module_name) -> void {
      // Loading a module.
      auto handle = dlopen(module_name.c_str(), RTLD_NOW | RTLD_LOCAL);
      if (handle != nullptr) {
        const auto error = dlerror();
        if (error == nullptr) {//<TODO> Adding error handle.
        }

        // Saving a new module.
        this->plugins.emplace(module_name, static_cast<void *>(handle));
      }
    };

    if (std::filesystem::is_regular_file(this->plugins_path)) {
      push_module(this->plugins_path);
    } else {
      for (const auto &module : std::filesystem::directory_iterator(this->plugins_path)) {
        if (std::filesystem::is_regular_file(module.status())) {
          // Adding a new module.
          push_module(module.path().string());
        }
      }
    }
  }

  /**
   * Destructor.
   */
  ~modules_loader() {
    for (auto &plugin : this->plugins) {
      // Closing loaded module.
      dlclose(plugin.second);
    }
  }

  modules_loader(const modules_loader &) = delete;
  modules_loader(const modules_loader &&) = delete;
  modules_loader &operator=(const modules_loader &) = delete;
  modules_loader &operator=(modules_loader &&) = delete;

  template<typename symbol_t>
  auto load(const char *symbol_name, std::function<void(symbol_t symbol, const std::string &module_name, const char *error)> onload) const -> void {
    for (const auto &plugin : this->plugins) {
      // Getting a symbol by it name.
      auto symbol = reinterpret_cast<symbol_t>(dlsym(plugin.second, symbol_name));

      if (onload != nullptr) {
        // Raising ONLOAD event.
        onload(reinterpret_cast<symbol_t>(symbol), plugin.first, dlerror());
      }
    }
  }
};
//-------------------------------------------------------------------------//
#endif // __MODULES_LOADER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
