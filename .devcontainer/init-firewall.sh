#!/bin/bash
set -euo pipefail  # Exit on error, undefined vars, and pipeline failures
IFS=$'\n\t'       # Stricter word splitting

# Fail CLOSED. `iptables -F` below flushes rules but leaves the chain policy
# at its default ACCEPT, and the DROP policy isn't set until the very end —
# after fetching GitHub's ranges and resolving every domain. Any of those
# steps can exit non-zero (network blip, DNS failure), which would otherwise
# leave the container with no rules and an ACCEPT policy: egress wide open.
# We can't just set DROP up front, because the allowlist is *built* by
# talking to the network. This trap is the guarantee instead: on any early
# error, drop to deny-all rather than exiting with egress open.
lockdown() {
    echo "ERROR: firewall setup failed before completion - locking down egress"
    iptables -P INPUT DROP
    iptables -P FORWARD DROP
    iptables -P OUTPUT DROP
}
trap lockdown ERR

# Flush only the two chains we actually add rules to (INPUT, OUTPUT) — never
# a blanket `iptables -F` (flushes every chain in the table, including
# whatever Docker put in FORWARD) and never `iptables -X` (deletes every
# custom chain in the table, including Docker's own).
#
# Our nested dockerd creates a DOCKER chain in BOTH the nat table (for the
# DNAT rule that makes published ports work) and the filter table (an ACCEPT
# rule on the forwarding path for that same published port), set up once at
# dockerd startup and never recreated later. A blanket flush deletes both —
# pulls kept working the whole time (no networking involved), but *launching*
# any container with a published port (Testcontainers' ryuk sidecar, in
# particular) failed with "iptables: No chain/target/match by that name",
# first for the nat-table chain, then — after fixing that — for the
# filter-table one, since both are equally destroyed by `-F`/`-X`.
#
# Leaving FORWARD's existing rules untouched doesn't weaken our own egress
# control: Docker's specific ACCEPT/DNAT rules there only cover its own
# container traffic, and FORWARD's policy (set to DROP below) still catches
# everything else. Our actual enforcement — the default-deny + allowlist —
# only needs to live in OUTPUT, which is the chain locally-originated traffic
# (i.e. this container's own processes, not traffic routed through it) goes
# through.
iptables -F INPUT
iptables -F OUTPUT
ipset destroy allowed-domains 2>/dev/null || true

# Re-open the policies before rebuilding. `-F` clears rules but NOT the chain
# policy, so on a re-run inside an already-firewalled container we would be
# left at DROP with an empty allowlist — and the allowlist is built by talking
# to the network, so the GitHub fetch below fails, the ERR trap fires, and the
# script bricks its own egress. That never shows up at container start (the
# policies are ACCEPT then), only when re-running the script in place.
#
# This does not weaken anything: it makes a re-run behave exactly like a fresh
# start, and the ERR trap plus the DROP policies set at the end still ensure we
# finish closed or not at all.
iptables -P INPUT ACCEPT
iptables -P FORWARD ACCEPT
iptables -P OUTPUT ACCEPT

# First allow DNS and localhost before any restrictions
# Allow outbound DNS
iptables -A OUTPUT -p udp --dport 53 -j ACCEPT
# Allow inbound DNS responses
iptables -A INPUT -p udp --sport 53 -j ACCEPT
# NOTE: no blanket outbound SSH rule. git-over-SSH to GitHub (git@github.com)
# is already covered by the allowed-domains allowlist below — GitHub's IP
# ranges are in the ipset, and the allowlist rule matches all ports, so
# tcp/22 to those hosts is permitted with responses via ESTABLISHED,RELATED.
# A blanket `--dport 22 -j ACCEPT` to any host would be an exfil channel that
# bypasses the allowlist, so it's deliberately omitted.
# Allow localhost
iptables -A INPUT -i lo -j ACCEPT
iptables -A OUTPUT -o lo -j ACCEPT

# Create ipset with CIDR support
ipset create allowed-domains hash:net

# Fetch GitHub meta information and aggregate + add their IP ranges
# (needed for git push/pull and `gh` — GitHub Actions/GHCR are not reachable
# from here since this is a local dev container, not CI)
echo "Fetching GitHub IP ranges..."
gh_ranges=$(curl -s https://api.github.com/meta)
if [ -z "$gh_ranges" ]; then
    echo "ERROR: Failed to fetch GitHub IP ranges"
    exit 1
fi

if ! echo "$gh_ranges" | jq -e '.web and .api and .git' >/dev/null; then
    echo "ERROR: GitHub API response missing required fields"
    exit 1
fi

echo "Processing GitHub IPs..."
while read -r cidr; do
    if [[ ! "$cidr" =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}/[0-9]{1,2}$ ]]; then
        echo "ERROR: Invalid CIDR range from GitHub meta: $cidr"
        exit 1
    fi
    echo "Adding GitHub range $cidr"
    ipset add allowed-domains "$cidr" -exist
done < <(echo "$gh_ranges" | jq -r '(.web + .api + .git)[]' | aggregate -q)

# Resolve and add other allowed domains.
#
# Deliberately NOT included: registry.npmjs.org, sentry.io, statsig.* —
# telemetry/auto-update traffic is disabled via containerEnv instead
# (CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC, DISABLE_AUTOUPDATER), so there's
# nothing that needs those domains open. Smaller allowlist, not a bigger one
# with things switched off behind it.
#
# Docker Hub domains (registry-1.docker.io, auth.docker.io,
# production.cloudflare.docker.com) are for `docker pull postgres:17`, used
# by Testcontainers and docker-compose.yml. NOTE: Docker Hub's blob storage
# is CDN-fronted and the backing domain has changed before without notice —
# if an image pull ever fails here, that's the first thing to re-check with
# `dig` and add. The postCreateCommand below pre-pulls postgres:17 once,
# before the firewall locks down, specifically to avoid depending on this at
# firewall-enforced runtime for the common case.
for domain in \
    "api.anthropic.com" \
    "repo.maven.apache.org" \
    "repo1.maven.org" \
    "plugins.gradle.org" \
    "services.gradle.org" \
    "registry-1.docker.io" \
    "auth.docker.io" \
    "production.cloudflare.docker.com" \
    "production.cloudfront.docker.com" \
    "marketplace.visualstudio.com" \
    "vscode.blob.core.windows.net" \
    "update.code.visualstudio.com"; do
    echo "Resolving $domain..."
    ips=$(dig +noall +answer A "$domain" | awk '$4 == "A" {print $5}')
    if [ -z "$ips" ]; then
        echo "ERROR: Failed to resolve $domain"
        exit 1
    fi

    while read -r ip; do
        if [[ ! "$ip" =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
            echo "ERROR: Invalid IP from DNS for $domain: $ip"
            exit 1
        fi
        echo "Adding $ip for $domain"
        ipset add allowed-domains "$ip" -exist
    done < <(echo "$ips")
done

# Get host IP from default route
HOST_IP=$(ip route | grep default | cut -d" " -f3)
if [ -z "$HOST_IP" ]; then
    echo "ERROR: Failed to detect host IP"
    exit 1
fi

HOST_NETWORK=$(echo "$HOST_IP" | sed "s/\.[0-9]*$/.0\/24/")
echo "Host network detected as: $HOST_NETWORK"

# Set up remaining iptables rules
iptables -A INPUT -s "$HOST_NETWORK" -j ACCEPT
iptables -A OUTPUT -d "$HOST_NETWORK" -j ACCEPT

# Set default policies to DROP first
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT DROP

# First allow established connections for already approved traffic
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
iptables -A OUTPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

# Then allow only specific outbound traffic to allowed domains
iptables -A OUTPUT -m set --match-set allowed-domains dst -j ACCEPT

# Reach containers on the *nested* daemon's bridges. The HOST_NETWORK rule
# above derives the OUTER Docker Desktop network (172.17/24) and never matches
# anything — under DinD the daemon we actually talk to uses 172.18/16 for its
# default bridge and allocates 172.19+ for each compose network.
#
# Without this, connecting to a published port via 127.0.0.1 fails in a way
# that looks nothing like a firewall: Docker's nat OUTPUT rule carries
# `! -d 127.0.0.0/8`, so loopback destinations get no DNAT and are served by
# the userland docker-proxy instead. The proxy accepts your connection, then
# makes its OWN hop to the container — which lands here and gets REJECTed. The
# client sees the connection accepted and then closed, i.e. an EOFException
# mid-handshake rather than "connection refused". That is what broke
# `./dev.sh` (bootRun -> jdbc:postgresql://localhost:5432) while Testcontainers
# kept working, since Testcontainers connects via the bridge gateway address,
# which is LOCAL-but-not-loopback and so does get DNAT'd in-kernel.
#
# Matched by interface rather than subnet so compose networks created after
# this script runs are covered too. This does not widen internet egress: these
# interfaces only reach containers on this machine, and FORWARD stays DROP.
iptables -A OUTPUT -o docker0 -j ACCEPT
iptables -A OUTPUT -o br+ -j ACCEPT

# Explicitly REJECT all other outbound traffic for immediate feedback
iptables -A OUTPUT -j REJECT --reject-with icmp-admin-prohibited

echo "Firewall configuration complete"
echo "Verifying firewall rules..."
if curl --connect-timeout 5 https://example.com >/dev/null 2>&1; then
    echo "ERROR: Firewall verification failed - was able to reach https://example.com"
    exit 1
else
    echo "Firewall verification passed - unable to reach https://example.com as expected"
fi

# Verify Anthropic API access
if ! curl --connect-timeout 5 https://api.anthropic.com >/dev/null 2>&1; then
    echo "ERROR: Firewall verification failed - unable to reach https://api.anthropic.com"
    exit 1
else
    echo "Firewall verification passed - able to reach https://api.anthropic.com as expected"
fi
