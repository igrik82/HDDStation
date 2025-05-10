#!/usr/bin/env bash

while (true); do
    # Deactivate
    deactivate 2>/dev/null

    # Clean old directories
    rm -rf .cache build venv compile_commands.json

    # Read current project name
    CURRENT_NAME=$(awk -F ' := ' '/PROJECT_NAME/ {print $2}' Makefile)

    echo -n "Enter new name. Current is (""$CURRENT_NAME""): "
    read -r PROJECT_NAME

    if [[ "$PROJECT_NAME" == "\n" || "$PROJECT_NAME" == *" "* ]]; then
        echo "There should be no spaces."
    else
        # Changing name in Makefile and CMakeLists
        sed -i "s/PROJECT_NAME := .*/PROJECT_NAME := ""$PROJECT_NAME""/g;" Makefile
        sed -i "s/project(.*)/project(""$PROJECT_NAME"")/g;" CMakeLists.txt

        # Make virtual enviroment
        python -m venv venv
        if [ -d "venv" ]; then
            source venv/bin/activate
            python -m pip install -r "$IDF_PATH"/requirements.txt

            # make compile comands
            DIRECTORY="build"

            # compile commands
            if [[ ! -d "$DIRECTORY" ]]; then
                mkdir "$DIRECTORY"
            fi

            cd $DIRECTORY || return
            cmake ../CMakeLists.txt
            cp compile_commands.json ../
            rm -rf build
            cd ..

            # Strip warning
            sed -i -E 's#-fstrict-volatile-bitfields##g' compile_commands.json
            sed -i -E 's#-mlongcalls##g' compile_commands.json

            make -j

            break
        fi
    fi
done
