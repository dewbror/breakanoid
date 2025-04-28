# Author: William Brorsson
# Date Created: Mars 10, 2025

SRC_C = $(shell find src -type f -name "*.c")
SRC_H = $(shell find src -type f -name "*.h")
SHADER_VERT = $(shell find src -type f -name "*.vert")
SHADER_FRAG = $(shell find src -type f -name "*.frag")
SHADER_COMP = $(shell find src -type f -name "*.comp")

# Make recipes for running clang-tidy and clang-format
.PHONY: tidy
tidy:
	clang-tidy $(SRC_H) $(SRC_C) -- -I./src -I./dep_repos/SDL/include -I./dep_repos/cglm/include -I./dep_repos/stb -IC:/VulkanSDK/1.4.304.1/Include

.PHONY: fmt
fmt:
	clang-format -i $(SRC_H) $(SRC_C)

# .PHONY: zip
# zip:
# 	@7z a -tzip $(GAME_NAME)_win64.zip "$(ROOT)$(RUN)" "./*.dll" "settings.cfg" "README.txt" "assets/*"

# .PHONY: tar
# # Add "*.so" when ive built the SDL shared object files.
# tar:
# 	@tar -cvzf $(GAME_NAME)_linux64.tar.gz "$(ROOT)$(RUN)" "settings.cfg" "README.txt" "assets/"
