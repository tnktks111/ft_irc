#ifndef ACOMMAND_HPP
#define ACOMMAND_HPP

#include "CommandContext.hpp"

class ACommand {
protected:
  ACommand();
  ACommand(const ACommand &other);
  ACommand &operator=(const ACommand &other);

public:
  virtual ~ACommand();
  virtual bool execute(CommandContext &ctx) = 0;
};

#endif
