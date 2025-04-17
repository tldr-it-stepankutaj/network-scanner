#!/bin/bash

_network-scanner_completions() {
  local cur="${COMP_WORDS[COMP_CWORD]}"
  local opts="--threads --mode --port --help --version"

  COMPREPLY=($(compgen -W "${opts}" -- "$cur"))
}

complete -F _ping_sweep_completions ping-sweep