#!/bin/bash

# List of repos to clone
REPOS=(
    git@github.com:recp/cglm.git
    git@github.com:libsdl-org/SDL.git
    git@github.com:nothings/stb.git
)

# Dir to clone repos into
TARGET_DIR="dep_repos"

# Create target dir if it doesnt exist
mkdir -p "$TARGET_DIR"

# Function to clone repos
clone_repo() {
    local repo_url=$1
    git clone --depth 1 "$repo_url" "$TARGET_DIR/$(basename "$repo_url" .git)"
}

# Iterate over the list of repos and clone them
for repo in "${REPOS[@]}"; do
    dir="$TARGET_DIR/$(basename "$repo_url" .git)"
    if [ -d "$dir" ]; then
        cd "$dir" || exit 1
        echo "Pulling $repo"
        git pull
        cd ..
    else
        echo "Cloning $repo"
        clone_repo "$repo"
    fi
done
