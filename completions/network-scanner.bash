#!/bin/bash

_network-scanner_completions() {
  local cur="${COMP_WORDS[COMP_CWORD]}"
  local opts="--threads --mode --port --help --version"

  # shellcheck disable=SC2207
  COMPREPLY=($(compgen -W "${opts}" -- "$cur"))

  # Add mode options
  if [[ ${cur} == --mode=* ]]; then
    local modes="icmp tcp fallback"
    local prefix=${cur%=*}=
    # shellcheck disable=SC2207
    COMPREPLY=($(compgen -P "$prefix" -W "$modes" -- "${cur#*=}"))
  fi
}

complete -F _network-scanner_completions network-scanner