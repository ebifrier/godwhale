help:
	@echo try targets as:
	@echo   cl
	@echo   cl-pgo
	@echo   icl
	@echo   icl-pgo

# Compile Options
#
# /DDEBUG (DEBUG)   builds release (debug) version of Bonanza.
# /DMINIMUM         disables some auxiliary functions that are not necessary to
#                   play a game, e.g., book composition and optimization of
#                   evaluation function.
# /DHAVE_SSE2       use SSE2 instructions for speed (N/A MS C/C++)
# /DHAVE_SSE4       use SSE2 and SSE4.1 instructions for speed (N/A MS C/C++)
# /DTLP             enables thread-level parallel search.
# /DMPV             enables multi-PV search.
# /DCSA_LAN         enables bonanza to talk CSA Shogi TCP/IP protcol.
# /DMNJ_LAN         enables client-mode of distributed computing.
# /DUSI             enables USI mode (not implemented).
# /DCSASHOGI        builds an engine for CSA Shogi (only Windows).
# /DNO_LOGGING      suppresses dumping log files.
# /DINANIWA_SHIFT   enables an Inaniwa strategy detection.
# /DDFPN            build the DFPN worker of mate-problems server.
# /DDFPN_CLIENT     enables the client-mode of mate-problem server.

FLAG = /DNDEBUG /DMINIMUM /DTLP /DHAVE_SSE2 /DINANIWA_SHIFT /DCSASHOGI /DNO_LOGGING

OBJS = data.obj main.obj io.obj proce.obj ini.obj utility.obj attack.obj\
       gencap.obj gennocap.obj gendrop.obj genevasn.obj mate3.obj genchk.obj\
       movgenex.obj makemove.obj unmake.obj time.obj csa.obj valid.obj\
       next.obj search.obj searchr.obj book.obj iterate.obj quiesrch.obj\
       swap.obj evaluate.obj root.obj hash.obj mate1ply.obj bitop.obj\
       rand.obj learn1.obj learn2.obj evaldiff.obj problem.obj ponder.obj\
       thread.obj sckt.obj debug.obj phash.obj dfpn.obj dfpnhash.obj

cl:
	$(MAKE) -f Makefile.vs bonanza.exe CC="cl" LD="link"\
		CFLAGS="$(FLAG) /MT /W4 /nologo /O2 /Ob2 /GS- /GL"\
		LDFLAGS="/NOLOGO /out:bonanza.exe /LTCG"

cl-pgo:
	$(MAKE) -f Makefile.vs clean
	$(MAKE) -f Makefile.vs bonanza.exe CC="cl" LD="link"\
		CFLAGS="$(FLAG) /MT /W4 /nologo /O2 /Ob2 /GS- /GL"\
		LDFLAGS="/NOLOGO /out:bonanza.exe /LTCG:PGI"
	$(MAKE) -f Makefile.vs pgo-run
	del bonanza.exe
	$(MAKE) -f Makefile.vs bonanza.exe LD="link"\
		LDFLAGS="/NOLOGO /out:bonanza.exe /LTCG:PGO"

icl:
	$(MAKE) -f Makefile.vs bonanza.exe CC="icl" LD="icl"\
		CFLAGS="/nologo $(FLAG) /Wall /O2 /Qipo"\
		LDFLAGS="/nologo /Febonanza.exe"

icl-pgo:
	$(MAKE) -f Makefile.vs clean
	$(MAKE) -f Makefile.vs bonanza.exe CC="icl" LD="icl"\
		CFLAGS="/nologo $(FLAG) /Wall /O2 /Qprof-gen"\
		LDFLAGS="/nologo /Febonanza.exe /Qprof-gen"
	$(MAKE) -f Makefile.vs pgo-run
	del *.obj bonanza.exe
	$(MAKE) -f Makefile.vs	bonanza.exe CC="icl" LD="icl"\
		CFLAGS="/nologo $(FLAG) /Wall /O2 /Qipo /Qprof-use"\
		LDFLAGS="/nologo /Febonanza.exe /Qprof-use"

pgo-run:
#	echo learn no-ini 32 32 1 2 2  >> runprof
	echo limit time extendable     >> runprof
	echo limit time 0 1            >> runprof
	echo tlp num 2                 >> runprof
	echo peek off                  >> runprof
	echo move 77                   >> runprof
	echo new                       >> runprof
	echo move 77                   >> runprof
	echo new                       >> runprof
	echo move 77                   >> runprof
	echo new                       >> runprof
	echo move 77                   >> runprof
	echo new                       >> runprof
	echo move 77                   >> runprof
	echo new                       >> runprof
	echo move 77                   >> runprof
	echo new                       >> runprof
	echo move 77                   >> runprof
	echo quit                      >> runprof
	bonanza.exe csa_shogi           < runprof

bonanza.exe : $(OBJS) bonanza.res
	$(LD) $(LDFLAGS) $(OBJS) bonanza.res User32.lib Ws2_32.lib

$(OBJS)  : shogi.h param.h bitop.h
dfpn.obj dfpnhash.obj: dfpn.h

bonanza.res : bonanza.rc bonanza.ico
	rc /fobonanza.res bonanza.rc

.c.obj :
	$(CC) $(CFLAGS) /c $*.c

clean :
	del /q runprof
        del /q *.pdb
        del /q *.ilk
        del /q *.pgd
        del /q *.pgc
        del /q *.dyn
	del /q *.obj
	del /q *.res
	del /q bonanza.exe
