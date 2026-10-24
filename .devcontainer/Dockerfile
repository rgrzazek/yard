FROM node:20

ARG TZ
ENV TZ="$TZ"

ARG CLAUDE_CODE_VERSION=latest

# Debian's base image has no locale configured, so Java's default charset
# falls back to ASCII — breaks on any non-ASCII character in test output
# (e.g. Cucumber report filenames derived from scenario names). C.UTF-8 is
# built into glibc, no `locale-gen`/`locales` package needed.
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# Install basic development tools. Java itself comes from the `java` devcontainer
# feature (see devcontainer.json), not from here — that keeps the JDK version in
# one place instead of two. iptables stays: dockerd needs it to manage the nested
# daemon's bridge NAT and published ports, independent of any firewall of ours.
RUN apt-get update && apt-get install -y --no-install-recommends \
  less \
  git \
  procps \
  sudo \
  fzf \
  zsh \
  man-db \
  unzip \
  gnupg2 \
  gh \
  iptables \
  iproute2 \
  dnsutils \
  jq \
  nano \
  vim \
  && apt-get clean && rm -rf /var/lib/apt/lists/*

# Ensure default node user has access to /usr/local/share
RUN mkdir -p /usr/local/share/npm-global && \
  chown -R node:node /usr/local/share

ARG USERNAME=node

# Persist bash history.
RUN SNIPPET="export PROMPT_COMMAND='history -a' && export HISTFILE=/commandhistory/.bash_history" \
  && mkdir /commandhistory \
  && touch /commandhistory/.bash_history \
  && chown -R $USERNAME /commandhistory

# Set `DEVCONTAINER` environment variable to help with orientation
ENV DEVCONTAINER=true

# Create workspace and config directories and set permissions
RUN mkdir -p /workspace /home/node/.claude && \
  chown -R node:node /workspace /home/node/.claude

WORKDIR /workspace

ARG GIT_DELTA_VERSION=0.18.2
RUN ARCH=$(dpkg --print-architecture) && \
  wget "https://github.com/dandavison/delta/releases/download/${GIT_DELTA_VERSION}/git-delta_${GIT_DELTA_VERSION}_${ARCH}.deb" && \
  sudo dpkg -i "git-delta_${GIT_DELTA_VERSION}_${ARCH}.deb" && \
  rm "git-delta_${GIT_DELTA_VERSION}_${ARCH}.deb"

# Set up non-root user
USER node

# Install global packages
ENV NPM_CONFIG_PREFIX=/usr/local/share/npm-global
ENV PATH=$PATH:/usr/local/share/npm-global/bin

# Set the default shell to zsh rather than sh
ENV SHELL=/bin/zsh

# Set the default editor and visual
ENV EDITOR=nano
ENV VISUAL=nano

# Default powerline10k theme
ARG ZSH_IN_DOCKER_VERSION=1.2.0
RUN sh -c "$(wget -O- https://github.com/deluan/zsh-in-docker/releases/download/v${ZSH_IN_DOCKER_VERSION}/zsh-in-docker.sh)" -- \
  -p git \
  -p fzf \
  -a "source /usr/share/doc/fzf/examples/key-bindings.zsh" \
  -a "source /usr/share/doc/fzf/examples/completion.zsh" \
  -a "export PROMPT_COMMAND='history -a' && export HISTFILE=/commandhistory/.bash_history" \
  -x

# Install Claude
RUN npm install -g @anthropic-ai/claude-code@${CLAUDE_CODE_VERSION}


# Deliberately leaving root as the image's default user. The docker-in-docker
# feature's entrypoint script (/usr/local/share/docker-init.sh) needs to run
# as root to start dockerd — if it isn't already root it falls back to
# `sudo /bin/sh -c "<dynamically-built dockerd command>"`, and that command
# string varies at runtime (DNS detection, address pool flags), so there's no
# meaningful way to scope a NOPASSWD sudoers rule to it — the only wildcard
# that would match is `/bin/sh -c *`, which is just unrestricted passwordless
# root. Leaving the image's default user as root only affects PID 1/entrypoint;
# it does NOT make your interactive session root — devcontainer.json's
# "remoteUser": "node" is what governs the shell you actually get and what
# `claude --dangerously-skip-permissions` runs as (which refuses to run as
# root regardless of what PID 1 is).
USER root
