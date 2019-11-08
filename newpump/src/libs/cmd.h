#ifndef __CHIP_COMMANDS_H__
#define __CHIP_COMMANDS_H__

#include <Arduino.h>
#include <algorithm>
#include <functional>
#include <map>
#include <vector>
#include "../ext/string/string.hpp"

using std::string;
using std::vector;

enum CommandSource {
    MQTT,
    HTTP,
    REST,
    UART,
    NONE
};

struct CommandParam {
  const string name;
  // name == args[0]
  vector<string> args;

  string toString() const;

  static const char* CMD_PREFIX;
  static const char* CMD_ARG_SEP;
  static const CommandParam INVALID;
  static bool hasValidPrefix(const string& cmdStr);
  static CommandParam parseArgs(const string& s);
};

using CMD_HANDLER_FUNC = std::function<void(const CommandParam&)>;

struct Command {
  const string name;
  const string desc;
  CMD_HANDLER_FUNC handler;

  string toString() const;
};

class CommandManager {
 public:
  bool handle(const CommandParam& param);
  void addCommand(Command* cmd);
  void addCommand(const char* name, const char* desc, CMD_HANDLER_FUNC handler);
  void addCommand(string& name, string& desc, CMD_HANDLER_FUNC handler);
  void removeCommand(Command* cmd);
  void removeCommand(string& name);
  vector<Command*> getCommands();
  String getHelpDoc();
  void setDefaultHandler(CMD_HANDLER_FUNC handler);

 private:
  CMD_HANDLER_FUNC _defaultHandler = nullptr;
  std::map<std::string, Command> _handlers;
  void _addHandler(Command* cmd);
  CMD_HANDLER_FUNC _getHandler(const string& name);
};

#endif