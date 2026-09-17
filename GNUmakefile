# Firing Range - short commands for the helper scripts.
#
#   make help        list the commands
#   make check       check the computer, the tools and the project
#   make setup       install what is missing, then build the project
#   make run         start the game
#
# Options go through ARGS:
#   make run ARGS=-fullscreen
#   make package ARGS=linux
#   make clean ARGS=all
#
# Why GNUmakefile and not Makefile: GNU make reads this name first, and on
# Linux Unreal's project file generator writes its own Makefile into the
# project root. With this name the two never collide.
#
# Windows has no make out of the box. Use fr.cmd there, or install make with
# "winget install ezwinports.make".

COMMANDS := help check setup build maps run editor package clean push

ifeq ($(OS),Windows_NT)
FR := powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/fr.ps1
else
FR := bash Scripts/fr.sh
endif

.DEFAULT_GOAL := help
.PHONY: $(COMMANDS)

$(COMMANDS):
	@$(FR) $@ $(ARGS)
