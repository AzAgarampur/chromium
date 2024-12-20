// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_COMMAND_H_
#define EXTENSIONS_COMMON_COMMAND_H_

#include <map>
#include <string>
#include <string_view>

#include "base/values.h"
#include "ui/base/accelerators/accelerator.h"

namespace extensions {

class Command {
 public:
  Command();
  Command(std::string_view command_name,
          std::u16string_view description,
          std::string_view accelerator,
          bool global);
  Command(const Command& other);
  ~Command();

  // The platform value for the Command.
  static std::string CommandPlatform();

  // Parse a string as an accelerator. If the accelerator is unparsable then
  // a generic ui::Accelerator object will be returns (with key_code Unknown).
  static ui::Accelerator StringToAccelerator(std::string_view accelerator,
                                             std::string_view command_name);

  // Returns the string representation of an accelerator without localizing the
  // shortcut text (like accelerator::GetShortcutText() does).
  static std::string AcceleratorToString(const ui::Accelerator& accelerator);

  // Return true if the |command_name| is one of the following action events:
  // Action Command Event, Browser Action Command Event, Page Action Command
  // Event.
  static bool IsActionRelatedCommand(std::string_view command_name);

  // Parse the command.
  bool Parse(const base::Value::Dict& command,
             std::string_view command_name,
             int index,
             std::u16string* error);

  // Accessors:
  const std::string& command_name() const { return command_name_; }
  const ui::Accelerator& accelerator() const { return accelerator_; }
  const std::u16string& description() const { return description_; }
  bool global() const { return global_; }

  // Setter:
  void set_accelerator(const ui::Accelerator& accelerator) {
    accelerator_ = accelerator;
  }
  void set_global(bool global) { global_ = global; }

 private:
  std::string command_name_;
  ui::Accelerator accelerator_;
  std::u16string description_;
  bool global_;
};

// A mapping of command name (std::string) to a command object.
using CommandMap = std::map<std::string, Command>;

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_COMMAND_H_
