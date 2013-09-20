----------------------------------------------------------------------
                        Bonanza 6.0 - Client Source Code
                                            Kunihito Hoki, May 2011
----------------------------------------------------------------------


1. Introduction
----------------

Bonanza is a state-of-the art computer shogi engine which runs on
Windows and Linux machines, and this directory contains a
platform-independent source code.

This source code is distributed with a hope that it will be useful in
addition to the main part of the shogi engine. This program includes
many useful functions such as manipulating a shogi board, reading and
writing a CSA record file, speaking of a CSA protocol with a socket
communication, and controlling time, and etc.. I believe this program
can be a good starting point if you are interested in making shogi
programs.

One main feature of this program is that it employs a brute-force
search method together with bitboard techniques as many chess programs
do. Another notable feature is a machine learning of shogi evaluation
functions. The details of the learning algorithm (aka Bonanza method)
were already presented [1], and this source code provides an example
of implementation of the learning method.

I admit that some parts of the source code are cryptic, e.g. codes in
"mate1ply.c". I hope that I will have some time to make a quality
documentation and comments on the program code, or someone else could
decrypt my program and provide a documentation.

Any comments or suggestions are welcome [2].

[1] K. Hoki, "Optimal control of minimax search results to learn
    positional evaluation", Game Programming Workshop, Hakone, Japan
    2008.

[2] Contact to "bonanza_query [at] hotmail.com".


2. Legal Notices
-----------------

This program is protected by copyright. Without a specific 
permission, any means of commercial applications are prohibited.
Furthermore, this program is distributed without any warranty.
Within these limits, you can use, redistribute, and/or modify it.


3. Change Logs
---------------

Version 6.0

Search() function is modified to ignore moves which receive negative
values from Static Exchange Evaluation (SEE) at frontier and
pre-frontier nodes. Self-play experiments showed that this modification
makes Bonanza slightly stronger. Similar results were also observed in
experiments using a shogi program YSS,
http://www32.ocn.ne.jp/~yss/horizon.txt. Moreover, the branch cut by
means of SEE at frontier nodes can also be found in a chess program,
Stockfish.

Now Bonanza does Late Move Reduction (LMR) at root nodes, and recursive
iterative deepening more often. Self-play experiments showed that these
modifications make Bonanza slightly stronger.

DFPN searcher is added to the set of Bonanza source codes (dfpn.c,
dfpn.h, and dfpnhash.c). You can use the searcher by using 'dfpn'
command with a compile option -DDFPN turned on. To make use of the DFPN
searcher in ordinary search, use 'dfpn_client' command with a compile
option -DDFPN_CLIENT turned on. To provide a proper service to the DFPN
client, you also need to use a server script 'src/server/dfpn_server.pl'
with one or more DFPN worker(s) connected with 'dfpn connect' command.
I am thankful to Dr. A. Nagai for his kind and fruitful advice.

Now Bonanza can be a client of parallel searches using cluster computing
environment with mnj command. 'src/server/parallel_server.pl' is a
server script of root-node parallelizations, and the server can be a
client of majority voting cluster computation.

Some techniques which increase computational efficiency of Bonanza are
reported by E. Ito, http://aleag.cocolog-nifty.com/. In his website,
two ideas are shown, i.e., (i) speed up of the computation of the static
evaluation function, and (2) use of SSE instructions of Intel processors
for bitboard operations. By adopting his ideas to Bonanza, NPS value
nearly doubled. This is amazing. To use SSE instructions, specify -DSSE2
or -DSSE4 compiler option. I am thankful to Mr. E. Ito for his kind and
fruitful advice.

-DINANIWA_SHIFT compilation flag is supplied to enable an Inaniwa-strategy detection

There are many other modifications and bug fixes.


Feliz 0.0

- Some results from Y. Sato's experiments with Bonanza inspired me
  to use the history table in LMR in Shogi, as Fruit did in chess. By
  doing so, some improvement in performance of the tree search is
  made.

- From private communications with Yaneurao, it turned out that the
  3-ply-mate detections in 'mate3.c' can search game trees more
  efficiently by using several heuristic pruning techniques. Now
  'mate3.c' adopts some of these techniques.

- E. Ito kindly reported two bugs in the source codes. One is in
  hash_store(), and the other is in ehash_probe() and
  ehash_store(). Now these are fixed.

- The structure of history table is modified from 'square-to' and
  'square-from' method to an exact match method by using the perfect
  hash technique. The hash function of moves in 'phash.c' is generated
  by codes at http://burtleburtle.net/bob/hash/perfect.html.

- A command 'mnj' is added to connect to the cluster-computing server
  in src/cluster/.

- In client_next_game(), the way of dealing with REJECT message from a
  CSA Shogi server is revised. Now Bonanza connects to the server again
  when the game is rejected by an opponent.


Version 4.1.3
- The owner of 'LS3600 Blog Webpage' pointed out that there were two
  serious bugs. Thank you very much! According to his indication,
  is_hand_eq_supe() in 'utility.c' and read_CSA_line() in 'csa.c' are
  revised.
- In MPV code in 'searchr.c', a test of 'root_abort' flag had been
  forgotten. Now the flag is tested after every call of search().
- 'lan.txt' is added to 'src/executable/' directory. This is an
  example of an input sequence to connect to CSA Shogi server.
- 'book.bin' now has smaller moves and positions than previous one does.
  Also, 'book_anti.csa' is added to 'src/executable/' directory. This
  is an example of bad moves which apear in records of human experts.
- 'Legal Notices' in this document is corrected.

Version 4.1.2
- In 'Makefile' and 'Makefile.vs', targets which require profile-guided
  optimization are removed. Furthermore, an option, which controls
  optimization, has been reverted from the aggressive flag, -O3, to a
  moderate one, '-O2'. These modifications were necessary for avoiding
  abnormal terminations of the program.
- In 'ini.c', the attribute of POSIX thread in a global-variable
  'pthread_attr' is set to 'detach-state'. Because threads will never
  join with any other threads in this program, the thread should be
  created in the detached state to free system resources.
- In 'ini.c', the default size of the transposition table is lifted
  from 12MByte to 48MByte.
- In 'iterate.c", probing the opening book is avoided when the move
  history of a current game has repetitions.
- In 'shogi.h', the margins of futility pruning are increased in
  accord with new positional evaluation of the feature vector in
  'fv.bin'.
- The quality of an opening book, 'winbin/book.bin', has been improved
  at the expense of quantity.

Version 4.1.0 and 4.1.1 (26 Apr 2009)
- In 'Makefile' and 'Makefile.vs', some options of Intel C compiler
  are modified. Here, agressive optimization '-O3' is substituted for
  the default '-O2', pthreads support '-pthread' is substituted for
  '-lpthread', and an obsolete '-static-libcxx' is removed.
- In 'Makefile', the conformance of GNU C and Intel C compilers are
  set to GNU extensions of ISO C99 by setting '-std=gnu99' because a
  POSIX function 'strtok_r()' is not in the C99 standard library.
- In 'Makefile', targets for GNU C with gprof and profile-guided
  optimization of GNU C are removed.
- In 'shogi.h', inline assemblies and intrinsics are used on x86-64 as
  well as x86. To detect the targets, pre-defined macros '__i386__'
  and '__x86_64__' are examined.
- In 'evaluate.c', the evaluation function looks up the table
  'stand_pat[ply]' to see if the position have been evaluated since
  the previous move is made by 'make_move_[bw]()' functions. Also, the
  evaluation function probes the hash table 'ehash_tbl[]' to avoid
  re-evaluation of the same position.
- In 'io.c', an immediate value 0 is substituted for 'fileno(stdin)'
  because the POSIX function 'fileno()' is not part of ANSI C. POSIX
  requires that the file descriptor associated with 'stdin' be 0.
- In 'iterate.c', a criterion for aging of transposition-table, i.e.,
  increment of a global variable 'trans_table_age', is lifted from 7%
  to 9% of saturation ratio of the table.
- In 'learn1.c', the macro 'SEARCH_DEPTH' is set to 2. The macro
  specifies the depth threashold of searches for finding all of
  principle variations of positions in a set of games.
- In 'learn1.c', a step size of increment/decrement of feature vectors
  is fixed to 1 in 'learn_parse2()' function.
- In 'hash.c', when a node can be pruned by using a hash value based on
  futility pruning, a return value of the node is set to beta.
- In 'rand.c', some codes of initialization of PRNG variables are
  moved to 'ini_genrand()' from 'ini()' function.
- In 'sckt.c', a function 'send_crlf()' is removed.
- In 'search.c', now 'search()' calls 'search_quies()' and returns if
  a search depth reaches a threashold. So that we can call 'search()'
  function anytime, and this makes source codes simple.
- In 'search.c', the static evaluation value is evaluated and used to
  see if the late-move reduction is applicable or not. The evaluation
  value is also used by the futility pruning.
- In 'search.c', the futility pruning is not applied if the node is in
  check or the move is a check.
- In 'search.c', now Bonanza sends the keep-alive command '0x0a' to
  the server in 'detect_signals()'.
- In 'time.c', 'set_seach_limit_time()' is simplified.
- In 'time.c', the second argument of 'gettimeofday()' is set to 'NULL'.

Version 4.0.4 (2 Feb 2009)
- An error of GCC inline assembly for spinlock in "thread.c" is fixed.
- In Windows OS, Bonanza now opens all streams with file sharing by
  using "_SH_DENYNO" constant in "io.c".
- GCC built-in functions are substituted for GCC inline assemblies for
  bit-scan operations in "bitop.h". Furthermore, "bitop.h" is removed,
  and some of macros in the header are integrated into "shogi.h".

Version 4.0.3 (Jan 2008)


4. Files
---------

Here is a list of files you can find in this directory.

C headers
- param.h     piece values
- shogi.h     main header
- bitop.h     bit operations
- dfpn.h      header of DFPN codes

basic C functions
- main.c      main function of C program
- data.c      definition of global variables
- ini.c       initializations
- rand.c      pseudo random number generator
- time.c      time functions
- bitop.c     bit operations
- utility.c   misc. functions

I/O
- proce.c     input procedures
- csa.c       csa file format I/O
- io.c        basic I/O
- sckt.c      Socket communications

bitboard manipulations
- attack.c    piece attacks
- genchk.c    move generation (checks)
- genevasn.c  move generation (evasions)
- gendrop.c   move generation (drops)
- gennocap.c  move generation (non-captures)
- gencap.c    move generation (captures)
- movgenex.c  move generation (inferior moves)
- makemove.c  make moves
- unmake.c    unmake move
- mate1ply.c  1-ply mate detection
- debug.c     examine bitboard validity

brute-force search
- iterate.c   iterative deepning search at root node
- searchr.c   alpha-beta search at root node
- search.c    alpha-beta search
- next.c      obtains next move
- quiesrch.c  quiescence search
- evaluate.c  static eveluation function
- evaldiff.c  easy and fast evaluation function
- swap.c      static exchange evaluation
- hash.c      transposition table
- phash.c     perfect hash function of moves
- thread.c    thread-level parallelization
- root.c      root move genelation and shallow min-max search
- mate3.c     3-ply mate detection
- ponder.c    pondering
- book.c      creates and probes opening book
- problem.c   auto problem solver
- valid.c     examine move validity

DFPN search
- dfpn.c      DFPN search
- dfpnhash.c  Hashing for DFPN search

optimal control of min-max search
- learn1.c    main functions
- learn2.c    feture vector manipuration

misc.
- bonanza.txt which now you are looking at
- Makefile    makefile for gnu make.exe
- Makefile.vs makefile for Microsoft nmake.exe
- bonanza.ico icon file for windows
- bonanza.rc  resource-definition file for windows
- lan.txt     example of input sequence to connect CSA Shogi server
- book_anti.csa example of a set of bad moves which apear in records
              of human exparts. This is used by 'book create' command.

4. How to build Bonanza
-----------------------

You can build Bonanza by means of GNU Make on Linux or Microsoft NMAKE
on Windows. Here is some examples:

- GCC on Linux
> make -f Makefile gcc

- Intel C++ Compiler on Linux
> make -f Makefile icc

- Microsoft C/C++ Compiler on Windows
> nmake -f Makefile.vs cl

- Intel C++ Compiler on Windows
> nmake -f Makefile.vs icl

The C source codes are written by using ANSI C plus a small number of
new features in ISO C99. Therefore, I think this can be easily built
in many platforms without much effort.

It may be necessary to define some macros in Makefile or
Makefile.vs. The macros are:

- NDEBUG (DEBUG)    builds release (debug) version of Bonanza

- MINIMUM           disables some auxiliary functions that are not
                    necessary to play a game, e.g., book composition
                    and optimization of evaluation functions

- TLP               enables thread-level parallel search

- MPV               enables multi-PV search

- CSA_LAN           enables Bonanza to communicate by CSA Shogi TCP/IP
                    protcol

- DEKUNOBOU         enables dekunobou interface (available only for
                    Windows)

- CSASHOGI          builds an engine for CSA Shogi (available only for
                    Windows)

- NO_LOGGING        suppresses dumping log files

- HAVE_SSE2         use SSE2 instructions for speed

- HAVE_SSE4         use SSE2 and SSE4.1 instructions for speed

- MNJ_LAN           enables a client-mode of cluster searches

- INANIWA_SHIFT     enables an Inaniwa strategy detection

- DFPN              build the DFPN worker of mate-problems server

- DFPN_CLIENT       enables the client-mode of mate-problem server


Bonanza is an application that does not provide graphical user
interface. If you could build "bonanza.exe" properly without CSASHOGI
macro, it shows a prompt "Black 1>" when you execute it at a computer
console.

Bonanza uses three binary files: a feature vector of static evaluation
function "fv.bin",  an opening book "book.bin", and a
position-learning database "hash.bin". You can find these in "winbin/"
directory. Without the NO_LOGGING option, Bonanza must find "log/"
directory to dump log files.


5. Command List
---------------

- beep on
- beep off
    These commands enable (on) or disable (off) a beep when Bonanza
    makes a move.  The default is on.

- book on
- book off
    These commands enable (on) or disable (off) to probe the opening
    book, "./book.bin".  The default is on.

- book narrow
- book wide
    When the command with "narrow" is used, Bonanza selects a book
    move from a small set of opening moves. The default is "wide". The
    narrowing of the opening moves is useful if you want Bonanza
    choose a common opening line.

- book create
    This command creates the opening book file, "./book.bin", by using
    numerous experts' games in a single CSA record file, "./book.csa".
    It also uses another CSA record file, "book_anti.csa", where you
    can register bad moves that may appear in the experts' games at
    the last moves in the record file. Here is the example:

    ----------------------------------------
    PI, +, +6978KI, %TORYO
    /
    PI, +, +6978KI, -8384FU, %TORYO
    /
    PI, +, +7776FU, -4132KI, %TORYO
    /
    PI, +, +7776FU, -4132KI, +2726FU, %TORYO
    ----------------------------------------

    This command becomes effective when MINIMUM macro is not defined
    in the Makefile.

- connect 'addr' 'port' 'id' 'passwd' ['ngame']
    This command connects Bonanza to a shogi server by using the CSA
    protocol. The first four arguments specify the network address,
    port number, user ID, and password, respectively. The last
    argument limits a number of games that will be played by Bonanza.
    This command becomes effective when CSA_LAN macro is defined in
    the Makefile.

- dfpn go
    Bonanza does DFPN search at a current position.

- dfpn hash 'num'
    This command is used to initialize the transposition table of
    DFPN search and set the size of the table to 2^'num'. This
    command becomes effective when DFPN macro is defined in the
    Makefile.

- dfpn connect 'hostname' 'port#' 'ID'
    This command is used to connect to the server script
    dfpn_server.pl as a worker. This command becomes effective when
    DFPN macro is defined in the Makefile.

- dfpn_client 'hostname' 'port#'
    This command is used to connect to the server script
    dfpn_server.pl as a client. With this, a root and its child
    posittions are examined by the DFPN worker(s). This command
    becomes effective when DFPN_CLIENT macro is defined in the
    Makefile.

- display ['num']
    This command prints the shogi board. If you want to flip the
    board, set 'num' to 2. If not, set it to 1.

- s
    Bonanza makes a prompt reply while thinking as soon as this
    command is used.

- hash 'num'
    This command is used to initialize the transposition table and
    set the size of the table to 2^'num'.

- hash learn create
    This command is used to make a zero-filled position-lerning
    database, "hash.bin". This command becomes effective when MINIMUM
    macro is not defined in the Makefile.

- hash learn on
- hash learn off
    These commands enable (on) or disable (off) the position learning.
    The default is on.

- learn 'str' 'steps' ['games' ['iterations' ['num1' ['num2']]]]
    This command optimizes a feature vector of the static evaluation
    function by using numorous experts' games in a single CSA record
    file, "./records.csa". If you want to use a zero-filled vector as
    an initial guess of the optimization procedure, set 'str' to
    "ini". If not, set it to "no-ini". The third argument 'games' is a
    number of games to be read from the record file. If the third
    argument is negative or omitted, all games are read from the file.

    The learning method iterates a set of procedures, and the number
    of iteration can be limited by the fourth argument. It continues
    as long as the argument is negative. The procedures consist of two
    parts. The first part reads the record file and creates principal
    variations by using 'num1' threads. The default value of 'num1' is
    1. The second part renews the feature vector 'steps' times by using
    'num2' threads in accord with the principal variations. The default
    value of 'steps' and 'num2' is 1. Note that each thread in the
    second procedure uses about 500MByte of the main memory. The two
    arguments 'num1' and 'num2' become effective when TLP macro is
    defined in the Makefile. After the procedures, the optimized
    vector is saved in "./fv.bin". This command become effective when
    MINIMUM macro is not defined in the Makefile.

- limit depth 'num'
    This command is used to specify a depth, 'num', at which Bonanza
    ends the iterative deepening search.

- limit nodes 'num'
    When this command is used, Bonanza stops thinking after searched
    nodes reach to 'num'.

- limit time 'minute' 'second' ['depth']
    This command limits thinking time of Bonanza. It tries to make
    each move by consuming the time 'minute'. When the time is spent
    all, it makes each move in 'second'. The last argument 'depth' can
    be used if you want Bonanza to stop thinking after the iterative
    deepening searches reach sufficient depth.

- limit time extendable
- limit time strict
    The command, "limit time extendable", allows Bonanza to think
    longer than the time limited by the previous command if it wishes
    to. The default is "strict".

- mnj 'sd' 'seed' 'addr' 'port' 'id' 'factor' 'stable_depth'
    This command connects Bonanza to the majority or parallel server in
    src/server/. The first two integers specify the standard
    deviation and initial seed of pseudo-random numbers which are
    added to the static evaluation function. Experiments suggested
    that an appropriate value for the standard deviation is 15. Note
    that all clients should use different seeds. The following three
    arguments are network address, port number, user ID,
    respectively. You can specify factor as a voting weight. Also
    stable_depth is useful to detect opening positions. Here, when
    Bonanza reaches the specified depth, then this is reported to the
    server to reduce thinking time. This command becomes effective
    when MNJ_LAN macro is defined in the Makefile.

- move ['str']
    Bonanza makes a move of 'str'. If the argument is omitted, Bonanza
    thinks of its next move by itself.

- mpv num 'nroot'
- mpv width 'threshold'
    These commands control the number of root moves, 'nroot', to
    constitute principal variations. The default number is 1. A root
    move that yields a smaller value than the best value by 'threshold'
    is neglected. The default threshold is about 200. These commands
    become effective when MPV macro is defined in the Makefile.

- new ['str']
    This command initializes the shogi board. The argument 'str'
    controls an initial configuration of the board.  If you want to
    play a no-handicapped game, set 'str' to "PI" and this is the
    default value. In a handicapped game, specify squares and pieces
    to drop, e.g. "new PI82HI22KA" or "new PI19KY".

- peek on
- peek off
    The command "peek on (off)" enables (disables) peeks at a buffer
    of the standard input file while Bonanza is thinking. The default
    is on. This command is useful when you want to process a set of
    commands as "> ./bonanza.exe < infile".

- ping
    Prompt Bonanza to print "pong".

- ponder on
- ponder off
    The command "ponder on (off)" enables (disables) thinks on the
    opponent's time. The default is on.

- problem ['num']
    This command is used to solve problems in "./problem.csa". Here
    is an example of the problem file.

    -----------------------------
    $ANSWER:+0024FU
    P1-KY-KE-OU-KI *  *  *  * -KY
    P2 *  *  *  *  * -KI *  *  * 
    P3 *  * -FU-GI-FU * -KE * -KA
    P4-FU *  * -FU-GI-FU-HI * -FU
    P5 *  *  *  *  *  *  * -FU+KY
    P6+FU+KA+FU+FU+GI+FU+KI *  * 
    P7 * +FU *  * +FU *  *  *  * 
    P8 * +OU+KI+GI *  * +HI *  * 
    P9+KY+KE *  *  *  *  * +KE * 
    P+00FU00FU
    P-00FU00FU00FU
    +
    /
    $ANSWER:+0087KY:+0088KY
    P1-OU-KE *  *  *  *  * +GI * 
    P2-KY-KI *  *  *  *  *  *  * 
    P3-FU-HI * -KI *  * -GI *  * 
    P4 *  * -KE *  *  *  *  * -FU
    P5 * +GI * -FU-FU-FU-FU-FU * 
    P6+FU+HI-FU *  *  *  *  *  * 
    P7 *  *  * +FU *  *  *  * +FU
    P8 *  * +OU+KI+KI *  *  *  * 
    P9+KY+KE *  *  *  *  * +KE+KY
    P+00KA00GI00KY00FU00FU
    P-00KA00FU00FU00FU00FU00FU
    +
    -----------------------------

    The argument 'num' specifies the number of problems to solve.

- quit
    The quit command and EOF character will exit Bonanza.

- read 'filename' [(t|nil) ['num']]
    This command is used to read a CSA record 'filename' up to 'num'
    moves. Set the second argument to "nil" when you want to ignore
    time information in the record. The default value is "t". Bonanza
    reads all move sequence if the last argument is neglected. If
    'filename' is ".", the command reads an ongoing game from the
    initial position.

- resign
    Use this command when you resign a game.

- resign 'num'
    This command specifies the threshold to resign. 'num' is a value
    of the threshold. The default is around 1000.

- stress on
- stress off
    When the command "stress on" is used, last-move shown in shogi
    board is stressed. The default is on.

- time remain 'num1' 'num2'
    This command tells Bonanza the remaining time. 'num1' ('num2') is
    the remaining time of black (white) in seconds.

- time response 'num'
    This command specifies a margin to control time. The time margin
    saves Bonanza from time up due to TCP/IP communication to a server
    program, sudden disc access, or imperfection of time control of
    Bonanza. 'num' is the time margin in milli-second. The default
    value is 200.

- tlp 'num'
    This command controls the number of threads to be created when
    Bonana considers a move to make. The command becomes effective
    when TLP macro is defined in the Makefile. 'num' is the number of
    threads. The default value is 1.

- #
    A line beginning with # causes all characters on that line
    to be ignored.

- [move command]
    A move command consists of four digits followed by two
    capital alphabets, e.g. 7776FU. The first two digits
    are a starting square and the last two are a target square. The
    starting square is "OO" if the  move is a dorp, e.g. 0087FU. The
    following two alphabets specify a piece type as the following,

      FU - pawn             (Fuhyo)       TO - promoted pawn    (Tokin)
      KY - lance            (Kyousha)     NY - promoted lance   (Narikyo)
      KE - knight           (Keima)       NK - promoted knight  (Narikei)
      GI - silver general   (Ginsho)      NG - promoted silver  (Narigin)
      KI - gold general     (Kinsyo)
      KA - Bishop           (Kakugyo)     UM - Dragon horse     (Ryuma)
      HI - Rook             (Hisha)       RY - Dragon king      (Ryuo)
      OU - King             (Osho)

    Here, words in parentheses are romanization of Japanese words.
