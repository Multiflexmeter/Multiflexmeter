#!/bin/bash
# writes the GIT hash into the file gitver.h
OUTFILE="../App/common/gitver.h"
GITSHA=$(git describe --tags --first-parent --dirty --always)
NEWCONTENT="\n#ifndef __GITVER_H\n#define __GITVER_H\n\n#define GIT_VER \"${GITSHA}\"\n\n#endif /* __GITVER_H */"
grep "${GITSHA}" $OUTFILE > /dev/null
if [ $? -ne 0 ]; then
  printf "$NEWCONTENT" > $OUTFILE
  echo "file $OUTFILE updated"
else
  echo "file content unchanged"
fi
