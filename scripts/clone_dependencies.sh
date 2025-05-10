#!/bin/bash

# List of repos to clone
REPOS=(
    git@github.com:recp/cglm.git
    git@github.com:nothings/stb.git
    git@github.com:libsdl-org/SDL.git
)

# Dir to clone repos into
TARGET_DIR="external"

# Create target dir if it doesnt exist
mkdir -p "$TARGET_DIR"

# Parse input arguemnts
SDL_ONLY=false
SDL_OFF=false

for arg in "$@"; do
    case "$arg" in
        SDL_ONLY)
            SDL_ONLY=true
            break
            ;;
        SDL_OFF)
            SDL_OFF=true
            break
            ;;
        *)
            echo "Unknown arguemnt: $arg"
            exit 1
            ;;
    esac
done

# Filter REPOS based on SDL_ONLY or SDL_OFF
if [ "$SDL_ONLY" = true ] && [ "$SDL_OFF" = true ]; then
    echo "Error: Both SDL_ONLY and SDL_OFF cannot be specified simultaneously."
    exit 1
elif [ "$SDL_ONLY" = true ]; then
    REPOS=(git@github.com:libsdl-org/SDL.git)
elif [ "$SDL_OFF" = true ]; then
    # Remove SDL from the REPOS array
    unset "REPOS[2]"
fi

# Iterate over the list of repos and clone them
for repo in "${REPOS[@]}"; do
    dir="$TARGET_DIR/$(basename "$repo" .git)"

    if [ -d "$dir" ]; then
        # If the repo already exists pull the latest changes
        echo "Pulling $repo"
        cd "$dir" || exit 1
        git pull
        cd "$OLDPWD" || exit 1
    else
        # If the repo doesnt exist, clone it
        # echo "Cloneing $repo"
        git clone --depth 1 "$repo" "$dir"
    fi
done
