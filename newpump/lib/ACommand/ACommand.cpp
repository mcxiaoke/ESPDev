#include "ACommand.h"

unsigned long CommandParam::_id = 0;

CommandParam::CommandParam(const string& name,
                           const vector<string> args,
                           const unsigned int id,
                           const CommandSource source,
                           const CMD_CALLBACK_FUNC callback)
    : name(name), args(args), id(id), source(source), callback(callback) {}

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

CommandParam CommandParam::from(const string& s) {
  vector<string> args = ext::string::split_any(s, CommandParam::CMD_ARG_SEP);
  for (auto arg : args) {
    ext::string::trim(arg);
  }
  // remove cmd prefix if has
  if (ext::string::contains_any(args[0], CommandParam::CMD_PREFIX)) {
    args[0].erase(0, 1);
  }
  // cmd to lower
  args[0] = ext::string::tolower(args[0]);
  CommandParam param{args[0], args};
  return param;
}

bool CommandManagerClass::handle(const CommandParam& param) {
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

void CommandManagerClass::addCommand(Command* cmd) {
  _addHandler(cmd);
}

void CommandManagerClass::addCommand(const string& name,
                                     const string& desc,
                                     CMD_HANDLER_FUNC handler) {
  Command cmd{name, desc, handler};
  _addHandler(&cmd);
}

void CommandManagerClass::removeCommand(Command* cmd) {
  _handlers.erase(cmd->name);
}

void CommandManagerClass::removeCommand(const string& name) {
  _handlers.erase(name);
}

vector<Command*> CommandManagerClass::getCommands() {
  vector<Command*> vs;
  vs.reserve(_handlers.size());
  for (auto& kvp : _handlers) {
    vs.push_back(&(kvp.second));
  }
  //   std::transform(_handlers.begin(), _handlers.end(), vs.begin(),
  //                  [](std::pair<const std::string, Command>& p) {
  //                    Serial.println(p.second.toString().c_str());
  //                    return &(p.second);
  //                  });
  return vs;
}

vector<string> CommandManagerClass::getCommandNames() {
  vector<string> vs;
  vs.reserve(_handlers.size());
  for (auto& kvp : _handlers) {
    vs.push_back(kvp.second.name);
  }
  return vs;
}

String CommandManagerClass::getHelpDoc() {
  String s("Commands: \n");
  for (auto const& kvp : _handlers) {
    auto const cmd = kvp.second;
    s += cmd.toString().c_str();
    s += "\n";
  }
  return s;
}

void CommandManagerClass::setDefaultHandler(CMD_HANDLER_FUNC handler) {
  _defaultHandler = handler;
}

void CommandManagerClass::_addHandler(Command* cmd) {
  _handlers.insert({cmd->name, *cmd});
}

CMD_HANDLER_FUNC CommandManagerClass::_getHandler(const string& name) {
  auto it = _handlers.find(name);
  return it != _handlers.end() ? it->second.handler : nullptr;
}

CommandManagerClass CommandManager;