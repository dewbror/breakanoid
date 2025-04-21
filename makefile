# Author: William Brorsson (aka s3cty, secty, sector)
# Date Created: Mars 10, 2025

# Make recipes for running clang-tidy and clang-format
.PHONY: tidy
tidy:
	clang-tidy src/*

.PHONY: format
format:
	clang-format -i src/**

# .PHONY: zip
# zip:
# 	@7z a -tzip $(GAME_NAME)_win64.zip "$(ROOT)$(RUN)" "./*.dll" "settings.cfg" "README.txt" "assets/*"

# .PHONY: tar
# # Add "*.so" when ive built the SDL shared object files.
# tar:
# 	@tar -cvzf $(GAME_NAME)_linux64.tar.gz "$(ROOT)$(RUN)" "settings.cfg" "README.txt" "assets/"
