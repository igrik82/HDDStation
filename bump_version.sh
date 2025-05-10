#!/bin/bash

set -e

# Protection against recursive execution
if [ -n "$BUMP_VERSION_RUNNING" ]; then
    exit 0
fi
export BUMP_VERSION_RUNNING=1

# Path to version file
VERSION_FILE="main/version.h"

# Get current branch name
BRANCH=$(git rev-parse --abbrev-ref HEAD)

# Restrict version bump to main or master branch only
if [[ "$BRANCH" != "main" && "$BRANCH" != "master" ]]; then
    echo "Error: You can only bump version from 'main' or 'master' branch."
    exit 1
fi

# Check for uncommitted changes (except during version commit)
if [[ -n "$(git status --porcelain)" && -z "$COMMITTING_VERSION" ]]; then
    echo "Error: There are uncommitted changes. Please commit or stash them before bumping version."
    exit 1
fi

# Force read from terminal even if stdin is closed (required for git hooks)
exec </dev/tty

echo "Current version tags:"
git tag -l --sort=-v:refname | head -5
echo ""

echo -n "Do you want to bump version? (y/N): "
read -r ANSWER

if [[ -z "$ANSWER" ]]; then
    ANSWER="N"
fi

if [[ "$ANSWER" =~ ^([Yy][Ee][Ss]|[Yy])$ ]]; then
    echo "Select version type:"
    echo "1) Patch (0.0.x)"
    echo "2) Minor (0.x.0)"
    echo "3) Major (x.0.0)"
    read -p "Choose an option (1/2/3): " VERSION_TYPE

    # Validate version type input
    if [[ ! "$VERSION_TYPE" =~ ^[1-3]$ ]]; then
        echo "Error: Invalid option. Please choose 1, 2, or 3."
        exit 1
    fi

    # Get latest tag
    LAST_TAG=$(git describe --tags $(git rev-list --tags --max-count=1) 2>/dev/null || true)

    if [[ -z "$LAST_TAG" ]]; then
        MAJOR=0
        MINOR=1
        PATCH=0
        echo "No existing tags found. Starting with version v0.1.0"
    else
        # Robust version parsing
        if [[ "$LAST_TAG" =~ ^v?([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
            MAJOR=${BASH_REMATCH[1]}
            MINOR=${BASH_REMATCH[2]}
            PATCH=${BASH_REMATCH[3]}
        else
            echo "Error: Last tag '$LAST_TAG' doesn't match version format (vX.Y.Z or X.Y.Z)"
            exit 1
        fi
        echo "Last version: $LAST_TAG"
    fi

    # Increment version based on selection
    case "$VERSION_TYPE" in
    1) PATCH=$((PATCH + 1)) ;;
    2)
        MINOR=$((MINOR + 1))
        PATCH=0
        ;;
    3)
        MAJOR=$((MAJOR + 1))
        MINOR=0
        PATCH=0
        ;;
    esac

    NEW_VERSION="v$MAJOR.$MINOR.$PATCH"
    echo "New version will be: $NEW_VERSION"

    # Create or update version.h file
    echo "Updating $VERSION_FILE..."
    mkdir -p "$(dirname "$VERSION_FILE")"
    cat >"$VERSION_FILE" <<EOF
#pragma once

#define VERSION_MAJOR $MAJOR
#define VERSION_MINOR $MINOR
#define VERSION_PATCH $PATCH

EOF

    git add "$VERSION_FILE"
    echo "Updated version in $VERSION_FILE to $NEW_VERSION"

    # Create annotated tag
    git tag -a "$NEW_VERSION" -m "Release $NEW_VERSION"
    echo "Created new tag: $NEW_VERSION"

    # Commit version changes
    export COMMITTING_VERSION=1
    git commit -m "Bump version to $NEW_VERSION"
    unset COMMITTING_VERSION

    # Handle push differently based on execution context
    if [ -n "$GIT_PUSH_FROM_HOOK" ]; then
        # When called from hook, push with --no-verify to prevent recursion
        git push origin "$BRANCH" --no-verify
        git push origin "$NEW_VERSION" --no-verify
    else
        # Normal push when executed manually
        git push origin "$BRANCH"
        git push origin "$NEW_VERSION"
    fi

    echo "Changes and tag pushed to remote."
else
    echo "Version bump cancelled."
fi
