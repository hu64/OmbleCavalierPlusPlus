#!/usr/bin/env bash
# set -euo pipefail

# Simple driver to run cutechess-cli matches between two engine builds.
# Defaults target the two folders you requested. Usage: ./run_cutechess.sh [options]

ENGINE_A="versions/omble_cavalier++_main"
ENGINE_B="versions/omble_cavalier++_pvs_and_bug_fix"
GAMES=100
TC="1+1"          # time control (base+increment), default: 3+0
CONCURRENCY=1      # number of concurrent games
THREADS=1          # UCI Threads per engine
PGNOUT="cutechess_results.pgn"
LOGOUT="cutechess_log.txt"
# RESIGN empty disables resigning; set to centipawn threshold to enable
RESIGN=""
REPEAT=true

usage(){
  cat <<USAGE
Usage: $0 [options]

Options:
  -a PATH   Path to engine A binary (default: ${ENGINE_A})
  -b PATH   Path to engine B binary (default: ${ENGINE_B})
  -g N      Number of games (default: ${GAMES})
  -t TC     Time control (default: ${TC})
  -c N      Concurrency (default: ${CONCURRENCY})
  -T N      Threads per engine (default: ${THREADS})
  -o FILE   PGN output file (default: ${PGNOUT})
  -l FILE   cutechess log file (default: ${LOGOUT})
  -r N      Resign threshold in centipawns (empty disables resign; default: disabled)
  -h        Show this help

Example:
  $0 -g 200 -t 3+2 -c 4
USAGE
}

while getopts "a:b:g:t:c:T:o:l:r:h" opt; do
  case ${opt} in
    a) ENGINE_A="$OPTARG" ;;
    b) ENGINE_B="$OPTARG" ;;
    g) GAMES="$OPTARG" ;;
    t) TC="$OPTARG" ;;
    c) CONCURRENCY="$OPTARG" ;;
    T) THREADS="$OPTARG" ;;
    o) PGNOUT="$OPTARG" ;;
    l) LOGOUT="$OPTARG" ;;
    r) RESIGN="$OPTARG" ;;
    h) usage; exit 0 ;;
    *) usage; exit 1 ;;
  esac
done

command -v cutechess-cli >/dev/null 2>&1 || { echo "cutechess-cli not found in PATH. Install it first." >&2; exit 1; }

# Verify engines exist
if [ ! -x "${ENGINE_A}" ]; then
  echo "Engine A not found or not executable: ${ENGINE_A}" >&2
  exit 1
fi
if [ ! -x "${ENGINE_B}" ]; then
  echo "Engine B not found or not executable: ${ENGINE_B}" >&2
  exit 1
fi

NAME_A=$(basename "${ENGINE_A}")
NAME_B=$(basename "${ENGINE_B}")

# Build cutechess-cli command explicitly (one element per argument)
CMD=(cutechess-cli)
CMD+=(-engine)
CMD+=("cmd=${ENGINE_A}")
CMD+=("name=A")
CMD+=("proto=uci")
# do not pass engine 'Threads' option; some builds don't expose it

CMD+=(-engine)
CMD+=("cmd=${ENGINE_B}")
CMD+=("name=B")
CMD+=("proto=uci")
CMD+=(-each)
CMD+=("proto=uci")
CMD+=("tc=${TC}")
# do not pass per-game 'threads' to engines; omit to avoid setoption errors

CMD+=(-games)
CMD+=("${GAMES}")
if [ "${REPEAT}" = true ]; then
  CMD+=(-repeat)
fi

CMD+=(-concurrency)
CMD+=("${CONCURRENCY}")
if [ -n "${RESIGN}" ]; then
  CMD+=(-resign)
  CMD+=("${RESIGN}")
fi

CMD+=(-draw)
CMD+=("movenumber=40")
CMD+=("movecount=6")
CMD+=("score=5")

CMD+=(-pgnout)
CMD+=("${PGNOUT}")

echo "Running: ${CMD[*]}"

# Run and tee output
"${CMD[@]}" 2>&1 | tee ${LOGOUT}

echo "Finished. PGN saved to ${PGNOUT}, log saved to ${LOGOUT}."
