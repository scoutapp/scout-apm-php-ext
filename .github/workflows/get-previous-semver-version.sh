#!/usr/bin/env bash

# Note, this doesn't work if the tags skip ANY versions, it expects all versions to be "sequential" and semver
# Also we only work with M.N.P (e.g. 1.2.3)

set -xeuo pipefail

VERSION="$1"

REGEX="([0-9]+)\.([0-9]+)\.([0-9]+)"

MAJOR=`echo $VERSION | sed -r -e "s/$REGEX/\1/"`
MINOR=`echo $VERSION | sed -r -e "s/$REGEX/\2/"`
PATCH=`echo $VERSION | sed -r -e "s/$REGEX/\3/"`

if [ "$PATCH" == "0" ]; then
  if [ "$MINOR" == "0" ]; then
    if [ "$MAJOR" == "0" ]; then
      echo "All versions for this release were zero" >&2
      exit 1
    else
      SERIES_GREP="$((MAJOR-1))."
    fi
  else
    SERIES_GREP="$MAJOR.$((MINOR-1))."
  fi
else
  SERIES_GREP="$MAJOR.$MINOR.$((PATCH-1))"
fi

set +e # allow failure here, as we check for empty string
NEWEST_TAG_IN_SERIES=`git tag -l --sort=-v:refname | grep "$SERIES_GREP" | head -n 1`
set -e

if [ "$NEWEST_TAG_IN_SERIES" == "" ]; then
  echo "Could not find a previous release before $VERSION" >&2
  exit 1
fi

echo $NEWEST_TAG_IN_SERIES
