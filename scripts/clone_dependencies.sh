#!/bin/bash

# List of repos to clone
REPOS=(
    "https://github.com/recp/cglm.git"
    "https://github.com/nothings/stb.git"
    "https://github.com/cimgui/cimgui.git"
    "https://github.com/clibs/cmocka.git"
    "https://github.com/libsdl-org/SDL.git"
)

REPO_TAGS=(
    v0.9.6
    master
    1.91.9
    1.1.5
    release-3.2.14
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
    REPOS=(https://github.com/libsdl-org/SDL.git)
    REPO_TAGS=(release-3.2.14)
elif [ "$SDL_OFF" = true ]; then
    # Remove SDL from the REPOS array
    unset "REPOS[5]"
    unset "REPO_TAGS[5]"
fi

i=0
# Iterate over the list of repos and clone them
for repo in "${REPOS[@]}"; do
    dir="$TARGET_DIR/$(basename "$repo" .git)"

    if [ -d "$dir" ]; then
        # If the repo already exists, do nothing
        echo "Directory $dir already exists"
    else
        # If the repo doesnt exist, clone it
        cd "$TARGET_DIR" || exit 1
        if git -c advice.detachedHead=false clone --recursive --depth 1 --branch "${REPO_TAGS[$i]}" "$repo"; then
            if [ "${REPO_TAGS[$i]}" != "master" ]; then
                echo "Note: switching to tag ${REPO_TAGS[$i]}"
            fi
        fi
        cd .. || exit 1
    fi
    i=$((i + 1))
done
