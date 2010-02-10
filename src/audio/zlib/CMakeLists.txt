SET(SOURCES
	zlib.h
	adler32.c   deflate.c  infback.c   inflate.c   minigzip.c  zconf.h     zutil.h
	compress.c  deflate.h  inffast.c   inflate.h   trees.c
	crc32.c     example.c  inffast.h   inftrees.c  trees.h
	crc32.h     gzio.c     inffixed.h  inftrees.h  uncompr.c   zutil.c
)

ADD_LIBRARY(our_zlib
	${SOURCES}
)
