#include "cmd.h"

string Command::toString() const {
  string s("/");
  s.append(name).append(" - ").append(desc);
  return s;
}

string CommandParam::toString() const {
  string s("{");
  for (auto const& arg : args) {
    s.append(" ").append(arg);
  }
  s.append("}");
  s.erase(1, 1);
  return s;
}

const char* CommandParam::CMD_PREFIX = "/#@!";
const char* CommandParam::CMD_ARG_SEP = ",:;| \t\n\r";
const CommandParam CommandParam::INVALID = {};

bool CommandParam::hasValidPrefix(const string& cmdStr) {
  //   LOGN("checkCommand");
  static const char* CMD_PREFIX = "/#@!$%";
  return cmdStr.length() > 2 && strchr(CMD_PREFIX, cmdStr.at(0)) != nullptr;
}

CommandParam CommandParam::parseArgs(const string& s) {
  vector<string> args = extstring::split_any(s, CommandParam::CMD_ARG_SEP);
  for (auto arg : args) {
    extstring::trim(arg);
  }
  // remove cmd prefix if has
  if (extstring::contains_any(args[0], CommandParam::CMD_PREFIX)) {
    args[0].erase(0, 1);
  }
  // cmd to lower
  args[0] = extstring::tolower(args[0]);
  CommandParam param{args[0], args};
  return param;
}

bool CommandManager::handle(const CommandParam& param) {
  auto handler = _getHandler(param.name);
  if (handler != nullptr) {
    handler(param);
    return true;
  } else {
    if (_defaultHandler != nullptr) {
      _defaultHandler(param);
      return true;
    }
  }
  return false;
}

void CommandManager::addCommand(Command* cmd) {
  _addHandler(cmd);
}

void CommandManager::addCommand(const char* name,
                                const char* desc,
                                CMD_HANDLER_FUNC handler) {
  Command cmd{name, desc, handler};
  _addHandler(&cmd);
}

void CommandManager::addCommand(string& name,
                                string& desc,
                                CMD_HANDLER_FUNC handler) {
  Command cmd{name, desc, handler};
  _addHandler(&cmd);
}

void CommandManager::removeCommand(Command* cmd) {
  _handlers.erase(cmd->name);
}

void CommandManager::removeCommand(string& name) {
  _handlers.erase(name);
}

vector<Command*> CommandManager::getCommands() {
  vector<Command*> vs;
  vs.reserve(_handlers.size());
  for (auto& kvp : _handlers) {
    vs.push_back(&(kvp.second));
  }
  return vs;
}

String CommandManager::getHelpDoc() {
  String s("Commands: \n");
  for (auto const& kvp : _handlers) {
    auto const cmd = kvp.second;
    s += cmd.toString().c_str();
    s += "\n";
  }
  return s;
}

void CommandManager::setDefaultHandler(CMD_HANDLER_FUNC handler) {
  _defaultHandler = handler;
}

void CommandManager::_addHandler(Command* cmd) {
  _handlers.insert(std::pair<std::string, Command>(cmd->name, *cmd));
}

CMD_HANDLER_FUNC CommandManager::_getHandler(const string& name) {
  auto it = _handlers.find(name);
  return it != _handlers.end() ? it->second.handler : nullptr;
}