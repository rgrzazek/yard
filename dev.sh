#!/usr/bin/env bash
# Starts the local dev stack (db + backend, and any frontend dev server
# added later) and tears all of it down together on exit — Ctrl-C, normal
# exit, or a crashed process.
set -uo pipefail

pids=()
cleaned_up=false

cleanup() {
    $cleaned_up && return
    cleaned_up=true
    trap - EXIT INT TERM
    echo
    echo "Stopping dev stack..."
    docker compose down
    kill "${pids[@]}" 2>/dev/null
    wait "${pids[@]}" 2>/dev/null
}
trap cleanup EXIT INT TERM

docker compose up -d db

./gradlew bootRun &
pids+=("$!")

echo "Run './gradlew -t classes' in another terminal for hot reload."

# Add a frontend dev server here the same way once one exists, e.g.:
# (cd frontend && npm run dev) &
# pids+=("$!")

# Portable stand-in for `wait -n` (bash 4.3+, which macOS's stock bash isn't):
# poll until any tracked job exits, then fall through to the EXIT trap.
while true; do
    for pid in "${pids[@]}"; do
        kill -0 "$pid" 2>/dev/null || exit
    done
    sleep 1
done
