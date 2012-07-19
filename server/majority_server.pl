#!/usr/bin/perl -w

# load perl modules
use strict;
use IO::Socket;
use IO::Select;
use IO::Handle;
use IO::File;
use Time::HiRes qw( time );
use File::Spec;
use Getopt::Long;

# autoflush STDOUT
$| = 1;

# subroutines
sub play_a_game      ($$$$$$);
sub parse_smsg       ($$$$);
sub parse_cmsg       ($$$$);
sub parse_clogin     ($$);
sub print_opinions   ($$$);
sub set_times        ($$);
sub clean_up_moves   ($$);
sub move_selection   ($$$$);
sub get_line         ($);
sub make_dir         ();
sub open_record      ($$);
sub open_log         ($);
sub open_sckts       ($$$$$);
sub out_csa          ($$$$);
sub select_sckts     ($$$$);
sub out_record       ($$) { print { $_[0] } "$_[1]\n" or die "$!"; }
sub out_log          ($$) { print { $_[0] } "$_[1]\n" or die "$!"; }
sub out_client       ($$) { print { $_[0] } "$_[1]\n" or die "$!"; }
sub out_clients      ($$$$);

# constants
sub phase_thinking  () { 0 }
sub phase_puzzling  () { 1 }
sub phase_pondering () { 2 }
sub tc_nmove        () { 33 }
sub tc_nmove1       () { 50 }
sub tc_nmove2       () { 20 }
sub sec_margin      () { 15 }
sub min_timeout     () { 0.05 }
sub max_timeout     () { 1.0 }
sub keep_alive      () { -1 }

{
    # defaults of command-line options
    my ( %status ) = ( client_port        => 4082,
		       client_num         => 3,
		       csa_host           => 'localhost',
		       csa_port           => 4081,
		       csa_id             => 'majority_vote',
		       csa_pw             => 'hoge-500-3-W',
		       sec_limit          => 600,
		       sec_limit_up       => 60,
		       time_response      => 0.2,
		       time_stable_min    => 2.0,
		       sec_spent_b        => 0,
		       sec_spent_w        => 0,
		       final_as_confident => q{},
		       audio              => q{},
		       buf_csa            => q{},
		       buf_record         => [],
		       buf_resume         => [] );

    # parse command-line options
    GetOptions( \%status,
		'client_port=i',
		'client_num=i',
		'csa_host=s',
		'csa_port=i',
		'csa_id=s',
		'csa_pw=s',
		'sec_limit=i',
		'sec_limit_up=i',
		'sec_spent_b=i',
		'sec_spent_w=i',
		'time_response=f',
		'time_stable_min=f',
		'audio!',
		'final_as_confident!' ) or die "$!";

    # creates a listening socket for my clients.
    my $sckt_listen
	= new IO::Socket::INET( LocalPort => $status{client_port},
				Listen    => SOMAXCONN,
				Proto     => 'tcp',
				ReuseAddr => 1 )
	or die "Can't create a listening socket: $!\n";
    
    my ( @sckt_clients );
    
    while ( 1 ) {
	my ( @game_summary );
	my $basename  = make_dir;
	my $fh_log    = open_log $basename;

	# open clients, CSA Shogi server, and receive GAME_SUMMARY.
	my $sckt_csa  = open_sckts( \%status, $sckt_listen, \@sckt_clients,
				    \@game_summary, $fh_log );


	my $fh_record = open_record \%status, $basename;

	play_a_game( \%status, $sckt_listen, \@sckt_clients, $sckt_csa,
		     $fh_record, $fh_log );
	
	# Shutdown the connection with CSA Shogi server.
	print $sckt_csa "LOGOUT\n"; # \n is assumed to be represented by 0x0a.
	out_log $fh_log, "csa< LOGOUT";
	while ( 1 ) {
	    my $input;
	    $sckt_csa->recv( $input, 65536 );
	    $input or last;
	}
	$sckt_csa->close;
	close $fh_record or die "$!";
	close $fh_log    or die "$!";
    }

    foreach my $sckt ( @sckt_clients ) { $sckt->close; }
}


sub play_a_game ($$$$$$) {
    my ( $ref_status, $sckt_listen, $ref_sckt_clients, $sckt_csa, $fh_record,
	 $fh_log ) = @_;
    my ( $line );

    # initialization of variables
    clean_up_moves $ref_status, $ref_sckt_clients;

    $$ref_status{timeout}        = max_timeout;
    $$ref_status{move_ponder}    = "";
    $$ref_status{time}           = time;
    $$ref_status{start_turn}     = $$ref_status{time};
    $$ref_status{start_think}    = $$ref_status{time};
    $$ref_status{time_printed}   = $$ref_status{time};
    $$ref_status{time_last_send} = $$ref_status{time};
    set_times $ref_status, $fh_log;

    if ( $$ref_status{phase} == phase_thinking ) {
	$$ref_status{timeout} = min_timeout;
    }

  LOOP: while ( 1 ) {

      # block until handles are ready to be read, or timeout
      my ( @sckts ) = select_sckts( $sckt_listen, $sckt_csa, $ref_status,
				    $ref_sckt_clients );
      # set current time
      $$ref_status{time} = time;
      
      # keep alive
      if ( keep_alive > 0
	   and $$ref_status{time} > ( keep_alive
				      + $$ref_status{time_last_send} ) ) {
	  out_csa $ref_status, $sckt_csa, $fh_log, "\n";
      }
      
      foreach my $sckt ( @sckts ) {

	  if ( $sckt == $sckt_csa ) {
	      
	      # received a message from server
	      parse_smsg( $ref_status, $ref_sckt_clients, $fh_record,
			  $fh_log ) or last LOOP;
	      next;
	      
	  }
	      
	  # received a message from one of the clients
	  parse_cmsg $ref_status, $sckt, $ref_sckt_clients, $fh_log;
      }
      
      # look at client's opinions to make a move or pondering-move
      move_selection $ref_status, $ref_sckt_clients, $sckt_csa, $fh_log;
  }
    
    # the game ends. now all of clients should idle away.
    out_clients $ref_status, $ref_sckt_clients, $fh_log, "idle";
}


sub parse_smsg ($$$$) {
    my ( $ref_status, $ref_sckt_clients, $fh_record, $fh_log ) = @_;

    my $line = get_line \$$ref_status{buf_csa};

    out_log $fh_log, "csa> $line";

    if ( $line =~ /^\#WIN/ )  { return 0; }
    if ( $line =~ /^\#LOSE/ ) { return 0; }
    if ( $line =~ /^\#DRAW/ ) { return 0; }

    if ( $line =~ /^\#\w/ )   { return 1; }
    if ( $line =~ /^\s*$/ )   { return 1; }
    if ( $line =~ /^\%\w/ ) {
	out_record $fh_record, $line;
	return 1;
    }

    # unless ( $line =~ /^([+-])(\d\d\d\d\w\w),T(\d+)/ ) { die "$!"; }
    if ( $line !~ /^([+-])(\d\d\d\d\w\w),T(\d+)/ ) { return 1; }

    my ( $color, $move, $sec ) = ( $1, $2, $3 );

    if ( $$ref_status{phase} == phase_thinking ) { die "$!"; }
    elsif ( $color eq $$ref_status{color} ) {

	# received time information from server, continue puzzling.
	$$ref_status{sec_mytime} += $sec;
	$$ref_status{timeout}     = min_timeout;
	out_record $fh_record, $line;
	out_log $fh_log, "Time: ${sec}s / $$ref_status{sec_mytime}s.\n";
	out_log $fh_log, "Opponent's turn starts.";
	set_times $ref_status, $fh_log;

    } elsif ( $$ref_status{phase} == phase_pondering
	      and $$ref_status{move_ponder} eq $move ) {
	    
	# received opp's move, pondering hit and my turn starts.
	my $time_think = $$ref_status{time}-$$ref_status{start_think};

	$$ref_status{sec_optime} += $sec;
	out_record $fh_record, $line;
	out_log $fh_log, "Opponent made a move $line.";
	out_log $fh_log, "Time: ${sec}s / $$ref_status{sec_optime}s.";
	out_log $fh_log, sprintf( "Pondering hit! (%.2fs)\n", $time_think );
	out_log $fh_log, "My turn starts.";

	$$ref_status{start_turn}  = $$ref_status{time};
	$$ref_status{phase}       = phase_thinking;
	$$ref_status{move_ponder} = "";
	$$ref_status{timeout}     = min_timeout;
	set_times $ref_status, $fh_log;

    } elsif ( $$ref_status{phase} == phase_pondering ) {
	    
	# received opp's move, pondering failed, my turn starts.
	$$ref_status{pid} += 1;
	out_clients( $ref_status, $ref_sckt_clients, $fh_log,
		     "alter $move $$ref_status{pid}" );

	$$ref_status{sec_optime} += $sec;
	out_record $fh_record, $line;
	out_log $fh_log, "pid is set to $$ref_status{pid}.";
	out_log $fh_log, "Opponent made an unexpected move $line.";
	out_log $fh_log, "Time: ${sec}s / $$ref_status{sec_optime}s.\n";
	out_log $fh_log, "My turn starts.";

	$$ref_status{start_turn}   = $$ref_status{time};
	$$ref_status{start_think}  = $$ref_status{time};
	$$ref_status{time_printed} = $$ref_status{time};
	$$ref_status{phase}        = phase_thinking;
	$$ref_status{move_ponder}  = "";
	$$ref_status{timeout}      = min_timeout;
	clean_up_moves $ref_status, $ref_sckt_clients;
	set_times $ref_status, $fh_log;

    } else {

	# received opp's move while puzzling, my turn starts.
	$$ref_status{pid} += 1;
	out_clients( $ref_status, $ref_sckt_clients, $fh_log,
		     "move $move $$ref_status{pid}" );

	$$ref_status{sec_optime} += $sec;
	out_record $fh_record, $line;
	out_log $fh_log, "pid is set to $$ref_status{pid}.";
	out_log $fh_log, "Opponent made a move $line while puzzling.";
	out_log $fh_log, "Time: ${sec}s / $$ref_status{sec_optime}s.\n";
	out_log $fh_log, "My turn starts.";
	
	$$ref_status{start_turn}   = $$ref_status{time};
	$$ref_status{start_think}  = $$ref_status{time};
	$$ref_status{time_printed} = $$ref_status{time};
	$$ref_status{phase}        = phase_thinking;
	$$ref_status{move_ponder}  = "";
	$$ref_status{timeout}      = min_timeout;
	clean_up_moves $ref_status, $ref_sckt_clients;
	set_times $ref_status, $fh_log;
    }

    return 1;
}


sub parse_cmsg ($$$$) {
    my ( $ref_status, $sckt, $ref_sckt_clients, $fh_log ) = @_;

    my $ref        = $$ref_status{$sckt};
    my $line       = get_line \$$ref{buf};

    unless ( $$ref{login} ) {

	parse_clogin $ref, $line;
	return;
    }

    my $time_think = $$ref_status{time}-$$ref_status{start_think};

#    print "$$ref{id}> $line";

    # keep alive
    if ( $line =~ /^\s*$/ ) {
	out_client $sckt, "";
	out_log $fh_log, "$$ref{id}< ";
	return;
    }

    unless ( $line =~ /pid=(\d+)/ ) {
	warn "pid from $$ref{id} is no match: $line\n";
	return;
    }

    # ignore delayed message
    if ( $1 != $$ref_status{pid} ) { return; }

    # ignore %TORYO move in puzzling phase
    if ( $line =~ /move=(%TORYO)/
	 and $$ref_status{phase} == phase_puzzling ) { return; }
    
    my $move      = undef;
    my $nodes     = undef;
    my $value     = undef;
    my $stable    = undef;
    my $final     = undef;
    my $confident = undef;


    if ( $line =~ /move=(%TORYO)/ )          { $move      = $1; }
    if ( $line =~ /move=(\d\d\d\d\w\w)/ )    { $move      = $1; }
    if ( $line =~ /v=([+-]?\d+)/ )           { $value     = $1; }
    if ( $line =~ /n=(\d+)/ )                { $nodes     = $1; }
    if ( $line =~ /stable/ ) {
	if ( defined $$ref{have_stable} )    { $stable    = 1; }
	else { warn "Invalid message 'stable' from $$ref{id}: $line\n";	}
    }
    if ( $line =~ /final/ ) {
	if ( defined $$ref{have_final} )     { $final     = 1; }
	else { warn "Invalid message 'final' from $$ref{id}: $line\n";	}
    }
    if ( $line =~ /confident/ ) {
	if ( defined $$ref{have_confident} ) { $confident = 1; }
	else { warn "Invalid message 'confident' from $$ref{id}: $line\n"; }
    }
    
    
    if ( defined $move and defined $nodes ) {

	$$ref{final}     = $final;
	$$ref{stable}    = $stable;
	$$ref{confident} = $confident;
	$$ref{move}      = $move;
	$$ref{nodes}     = $nodes;
	$$ref{spent}     = $time_think;
	if ( defined $value ) { $$ref{value} = $value; }

    } elsif ( defined $final and defined $$ref{move} ) {

	$$ref{final}     = $final;
	$$ref{spent}     = $time_think;
	if ( defined $value ) { $$ref{value} = $value; }

    } elsif ( defined $stable and defined $$ref{move} ) {

	$$ref{stable}    = $stable;
	$$ref{spent}     = $time_think;
	if ( defined $value ) { $$ref{value} = $value; }

    } elsif ( defined $confident and defined $$ref{move} ) {

	$$ref{confident} = $confident;
	$$ref{spent}     = $time_think;
	if ( defined $value ) { $$ref{value} = $value; }

    } elsif ( defined $nodes and defined $$ref{move} ) {

	$$ref{nodes}     = $nodes;
	$$ref{spent}     = $time_think;
	if ( defined $value ) { $$ref{value} = $value; }

    } else { warn "Invalid message from $$ref{id}: $line\n"; }

    my ( @boxes );
    my $nvalid = 0;

    # Organize all opinions into the ballot boxes.
    # Each box contains the same move-opinions.
    foreach my $sckt ( @$ref_sckt_clients ) {
	my $i;
	my $ref = $$ref_status{$sckt};

	if ( not defined $$ref{move} ) { next; }

	$nvalid += 1;
	for ( $i = 0; $i < @boxes; $i++ ) {
	    my $op = ${$boxes[$i]}[1];
	    if ( $$op{move} eq $$ref{move} ) { last; }
	}
	
	${$boxes[$i]}[0] += ( defined $$ref{resume} ) ? 0.0 : $$ref{factor};
	push @{$boxes[$i]}, $ref;
    }
    
    # Sort opinions by factor
    @boxes = sort { $$b[0] <=> $$a[0] } @boxes;

    $$ref_status{boxes}  = \@boxes;
    $$ref_status{nvalid} = $nvalid;
}


sub move_selection ($$$$) {
    my ( $ref_status, $ref_sckt_clients, $sckt_csa, $fh_log ) = @_;
    my ( $move_ready );
	  
    if ( $$ref_status{time_printed} + 60 < $$ref_status{time} ) {

	$$ref_status{time_printed} = $$ref_status{time};
	print_opinions $$ref_status{boxes}, $$ref_status{nvalid}, $fh_log;
	out_log $fh_log, "";
    }

    if ( $$ref_status{phase} == phase_pondering ) { return; }


    my $time_turn  = $$ref_status{time} - $$ref_status{start_turn};
    my $time_think = $$ref_status{time} - $$ref_status{start_think};

    # see if there are any confident decisions or not.
    foreach my $sckt ( @$ref_sckt_clients ) {
	    
	my $ref = $$ref_status{$sckt};
	    
	if ( ( $ref_status->{final_as_confident} and defined $$ref{final} )
	     or defined $$ref{confident} ) {

	    my $value_str;
	    if ( defined $$ref{value} ) { $value_str = " $$ref{value}"; }
	    else                        { $value_str = q{}; }
	    out_log( $fh_log,
		     "$$ref{id} is confident in $$ref{move}"
		     . " ($value_str)." );
	    $move_ready = $$ref{move};
	    last;
	}
    }


    # Find the best move from the ballot box.
    if ( not $move_ready
	 and defined $$ref_status{boxes}
	 and defined ${${$$ref_status{boxes}}[0]}[0] ) {

	my $ref_boxes   = $$ref_status{boxes};
	my $ops         = $$ref_boxes[0];
	my $op          = $$ops[1];
	my $nop         = @$ops - 1; # "-1" to ignore the 1st element
	my $nvalid      = $$ref_status{nvalid};
	my $condition   = 0;
	my $sec_elapsed;

	# check time
	if ( $$ref_status{phase} == phase_thinking
	     and ( $$ref_status{sec_mytime}
		   + int( $$ref_status{sec_fine} ) + 1.0
		   >= $$ref_status{sec_limit} ) ) {
	    # in byo-yomi
	    $sec_elapsed = $time_turn;

	} else {

	    $sec_elapsed = ( $time_turn + int( $time_think - $time_turn ) );
	}

	if ( $nvalid > 2 and $nop > $nvalid * 0.90
	     and $sec_elapsed > $$ref_status{sec_easy} ) {

	    out_log $fh_log, "Easy Move";
	    $condition = 1;

	} elsif ( $nop > $nvalid * 0.70
		  and $sec_elapsed > $$ref_status{sec_fine} ) {
	    
	    out_log $fh_log, "Normal Move";
	    $condition = 1;

	} elsif ( $sec_elapsed > $$ref_status{sec_max} ) {

	    out_log $fh_log, "Difficult Move";
	    $condition = 1;
	}


	# check stable
	if ( not $condition
	     and ( $$ref_status{time_stable_min}
		   < $time_think + $$ref_status{time_response} ) ) {

	    my $nhave_stable = 0;
	    my $nstable      = 0;

	    foreach my $sckt ( @$ref_sckt_clients ) {
	    
		my $ref = $$ref_status{$sckt};

		unless ( defined $$ref{have_stable} ) { next; }
		
		$nhave_stable += $$ref{factor};
		if ( defined $$ref{stable} ) { $nstable += $$ref{factor}; }
	    }

	    if ( $nhave_stable < $nstable * 2 ) { $condition = 1; }
	}


	# see if there is any final decisions or not.
	if ( not $condition and 1.0 < $sec_elapsed ) {
	    
	    my %nfinal;
	    my $not_final   = 0;
	    
	    foreach my $sckt ( @$ref_sckt_clients ) {
		
		my $ref = $$ref_status{$sckt};
		
		unless ( defined $$ref{have_final} ) { next; }
		
		if ( $$ref{final} ) { $nfinal{$$ref{move}} += $$ref{factor}; }
		else                { $not_final           += $$ref{factor}; } 
	    }

	    my ( $first, $second ) = sort { $b <=> $a } values %nfinal;
	    if ( defined $first ) {
		$second = 0.0 unless defined $second;
		$second += $not_final;
		
		if ( $first >= $second + $not_final ) { $condition = 1; }
	    }
	}
	
	if ( $condition ) {
	    
	    out_log $fh_log, "The best move is ${$op}{move}.";
	    print_opinions $$ref_status{boxes}, $$ref_status{nvalid}, $fh_log;
	    
	    $move_ready = ${$op}{move};
	}
    }
    
    unless ( $move_ready ) { return; }


    # A move is found.
    if ( $$ref_status{phase} == phase_puzzling ) {
	      
	# Make a move, and ponering start.
	$$ref_status{pid} += 1;
	my $color = ( $$ref_status{color} eq '+' ) ? '-' : '+';

	out_clients( $ref_status, $ref_sckt_clients, $fh_log,
		     "move $move_ready $$ref_status{pid}" );
	    
	out_log $fh_log, "pid is set to $$ref_status{pid}.";
	out_log $fh_log, "Ponder on $color$move_ready.";
	    
	clean_up_moves $ref_status, $ref_sckt_clients;
	$$ref_status{move_ponder}  = $move_ready;
	$$ref_status{start_think}  = $$ref_status{time};
	$$ref_status{time_printed} = $$ref_status{time};
	$$ref_status{timeout}      = max_timeout;
	$$ref_status{phase}        = phase_pondering;
	return;
    }
	
    # Make a move, and start puzzling.
    $$ref_status{pid} += 1;
    my $csa_move = ( $move_ready !~ /^%/) ? $$ref_status{color} : "";
    $csa_move .= $move_ready;
    out_csa $ref_status, $sckt_csa, $fh_log, $csa_move;
    if ( $$ref_status{audio} ) { print "\a"; }
    if ( $move_ready !~ /^%/ ) {
      out_clients( $ref_status, $ref_sckt_clients, $fh_log,
		   "move $move_ready $$ref_status{pid}" );
    }
    out_log $fh_log, "pid is set to $$ref_status{pid}.";
    out_log $fh_log, sprintf( "time-searched: %7.2fs", $time_think );
    out_log $fh_log, sprintf( "time-elapsed:  %7.2fs", $time_turn );
	
    clean_up_moves $ref_status, $ref_sckt_clients;
    $$ref_status{start_turn}   = $$ref_status{time};
    $$ref_status{start_think}  = $$ref_status{time};
    $$ref_status{time_printed} = $$ref_status{time};
    $$ref_status{phase}        = phase_puzzling;
    $$ref_status{timeout}      = max_timeout;
}


# input
#   $$ref_status{sec_mytime}
#   $$ref_status{sec_limit}
#   $$ref_status{sec_limit_up}
#   $$ref_status{time_response}
#   $$ref_status{start_turn}
#   $$ref_status{start_think}
#   $$ref_status{phase}
#   tc_nmove
#   sec_margin
#
# output
#   $$ref_status{sec_max}
#   $$ref_status{sec_fine}
#   $$ref_status{sec_easy}
#
sub set_times ($$) {
    my ( $ref_status, $fh_log ) = @_;
    my ( $sec_left, $sec_fine, $sec_max, $sec_easy, $max, $min );

    # set $sec_max and $sec_fine
    #    $sec_max  - maximum allowed time-consumption
    #    $sec_fine - normal time consumption
    #    $sec_easy - short time consumption
    $sec_left = $$ref_status{sec_limit} - $$ref_status{sec_mytime};

    if ( $$ref_status{sec_limit_up} ) {

	my $sec_ponder
	    = int $$ref_status{start_turn} - $$ref_status{start_think};
	# have byo-yomi
	if ( $sec_left < 0 ) { $sec_left = 0; }

	$sec_fine  = int( $$ref_status{sec_limit} / tc_nmove + 0.5 );
	$sec_fine -= $sec_ponder;
	if ( $sec_fine < 0 ) { $sec_fine = 0; }

	# t = 2s is not beneficial since 2.8s are almost the same as 1.8s.
        # So that we rather want to use up the ordinary time.
	if ( $sec_fine < 3 ) { $sec_fine = 3; }

	# 'byo-yomi' is so long that the ordinary time-limit is negligible.
	if ( $sec_fine < $$ref_status{sec_limit_up} * 2.0 ) {
	    $sec_fine = int( $$ref_status{sec_limit_up} * 2.0 );
	}

	$sec_max  = $sec_fine * 3.0;
	$sec_easy = int( $sec_fine / 3.0 + 0.5 );

    } else {

	# no byo-yomi
	if ( $$ref_status{sec_mytime}+sec_margin < $$ref_status{sec_limit} ) {

#	    $sec_fine = int( $sec_left / tc_nmove + 0.5 );
	    my $sec1 = $$ref_status{sec_limit} / tc_nmove1;
	    my $sec2 = $sec_left               / tc_nmove2;
	    if ( $sec2 < $sec1 ) { $sec1 = $sec2; }
	    $sec_fine = int( $sec1 + 0.5 );
	    $sec_max  = $sec_fine * 3.0;
	    $sec_easy = int( $sec_fine / 3.0 + 0.5 );

	    # t = 2s is not beneficial since 2.8s are almost the same as 1.8s.
	    # So that we rather want to save time.
	    if ( $sec_easy < 3 ) { $sec_easy = 1; }
	    if ( $sec_fine < 3 ) { $sec_fine = 1; }
	    if ( $sec_max  < 3 ) { $sec_max  = 1; }

	} else { $sec_fine = $sec_max = 1; }
    }

    $max = $sec_left + $$ref_status{sec_limit_up};
    $min = 1;

    if ( $max < $sec_easy ) { $sec_easy = $max; }
    if ( $max < $sec_fine ) { $sec_fine = $max; }
    if ( $max < $sec_max )  { $sec_max  = $max; }
    if ( $min > $sec_easy ) { $sec_easy = $min; }
    if ( $min > $sec_fine ) { $sec_fine = $min; }
    if ( $min > $sec_max )  { $sec_max  = $min; }
    
    $$ref_status{sec_max}  = 1.0 - $$ref_status{time_response} + $sec_max;
    $$ref_status{sec_fine} = 1.0 - $$ref_status{time_response} + $sec_fine;
    $$ref_status{sec_easy} = 1.0 - $$ref_status{time_response} + $sec_easy;

    if ( $$ref_status{phase} == phase_puzzling ) {

	$$ref_status{sec_max}  /= 5.0;
	$$ref_status{sec_fine} /= 5.0;
	$$ref_status{sec_easy} /= 5.0;
    }

    out_log $fh_log, sprintf( "Time limits: max=%.2f fine=%.2f easy=%.2fs",
			      $$ref_status{sec_max}, $$ref_status{sec_fine},
			      $$ref_status{sec_easy} );
}


sub clean_up_moves ($$) {
    my ( $ref_status, $ref_sckt_clients ) = @_;

    delete $$ref_status{boxes};
    foreach my $sckt ( @$ref_sckt_clients ) {
	my $ref = $$ref_status{$sckt};
	delete @$ref{ qw(move stable final confident resume value) };
    }
}


sub make_dir () {
    my ( $sec, $min, $hour, $mday, $mon, $year ) = localtime( time );

    $year -= 100;
    $mon  += 1;
    my $dirname = sprintf "%02s%02s", $year, $mon;
    unless ( -d $dirname ) { mkdir $dirname or die "$!"; }

    my $basename  = sprintf "%02s%02s%02s%02s", $mday, $hour, $min, $sec;

    return File::Spec->catfile( $dirname, $basename );
}


sub print_opinions ($$$) {
    my ( $ref_boxes, $nvalid, $fh_log ) = @_;

    if ( defined $nvalid ) {
	out_log $fh_log, "$nvalid valid ballots are found.";
    }

    foreach my $ops ( @$ref_boxes ) {
	my ( $sum, @ops_ ) = @$ops;

	out_log $fh_log, "sum = $sum";
	foreach my $op ( @ops_ ) {
	    my $spent = $$op{spent} + 0.001;
	    my $nps = $$op{nodes} / $spent / 1000.0;
	    my $str = sprintf( "  %.2f %s nps=%7.1fK %6.1fs %s",
			       $$op{factor}, $$op{move}, $nps, $$op{spent},
			       $$op{id} );

	    if ( defined $$op{value} )  { $str .= " $$op{value}"; }
	    if ( $$op{stable} )         { $str .= " stable"; }
	    if ( $$op{final} )          { $str .= " final"; }
	    if ( $$op{resume} )         { $str .= " resume"; }
	    out_log $fh_log, $str;
	}
    }
}


sub open_record ($$) {
    my ( $ref_status, $basename ) = @_;
    my ( $fh_record );

    open $fh_record, ">${basename}.csa" or die "$!";
    $fh_record->autoflush(1);

    out_record $fh_record, "N+$$ref_status{name1}";
    out_record $fh_record, "N-$$ref_status{name2}";
    out_record $fh_record, "PI";
    out_record $fh_record, "+";
    foreach my $line ( @{$$ref_status{buf_record}} ) {
	out_record $fh_record, $line;
    }

    return $fh_record;
}


sub open_log ($) {
    my ( $basename ) = @_;
    my ( $fh_log );

    open $fh_log, ">${basename}.log" or die "$!";
    $fh_log->autoflush(1);

    return $fh_log;
}


sub open_sckts ($$$$$) {
    my ( $ref_status, $sckt_listen, $ref_sckt_clients,
	 $ref_game_summary, $fh_log ) = @_;
    my ( $selector, $sckt_csa );

    print "Clients connected:\n";
    foreach my $sckt ( @$ref_sckt_clients ) {

	my $ref = $$ref_status{$sckt};

	print "  $$ref{id} $$ref{factor}";
	print " stable"    if $$ref{have_stable};
	print " final"     if $$ref{have_final};
	print " confident" if $$ref{have_confident};
	print "\n";
    }

    # wait for a certain number of clients connects to me.
    print "Wait for $$ref_status{client_num} clients connect to me\n";

    $$ref_status{timeout}  = max_timeout;
    my $game_agreed        = undef;
    my $time_connect_tried = time - 30.0;

    while ( not $game_agreed ) {

	my $nlogin = grep { ${$$ref_status{$_}}{login} } @$ref_sckt_clients;
	
	if ( not $sckt_csa
	     and $nlogin >= $$ref_status{client_num}
	     and time >= $time_connect_tried + 30.0 ) {

	    # connect() and LOGIN to server
	    print "Try connect() to CSA Server.\n";
	    $time_connect_tried = time;
	    $sckt_csa
		= new IO::Socket::INET( PeerAddr => $$ref_status{csa_host},
					PeerPort => $$ref_status{csa_port},
					Proto    => 'tcp' );
	    
	    if ( $sckt_csa ) {

		out_csa( $ref_status, $sckt_csa, *STDOUT,
			 "LOGIN $$ref_status{csa_id} $$ref_status{csa_pw}" );

	    } else { warn "$!"; }
	}

	foreach my $sckt ( select_sckts( $sckt_listen, $sckt_csa, $ref_status,
					 $ref_sckt_clients ) ) {

	    if ( grep { $sckt == $_ } @$ref_sckt_clients ) {

		# message received from one client
		my $ref  = $$ref_status{$sckt};
		my $line = get_line \$$ref{buf};
		if ( $$ref{login} ) { next; }
		
		parse_clogin $ref, $line;
		next;
	    }

	    # message received from server
	    my $line = get_line \$$ref_status{buf_csa};
	    out_log $fh_log, "csa> $line";
	    
	    if ( $line =~ /^LOGIN:incorrect/ ) { die "$!"; }
	    
	    if ( $line =~ /^LOGIN:$$ref_status{csa_id} OK$/ ) { next; }
	    
	    if ( $line =~ /^BEGIN Game_Summary/ ) { next; }

	    if ( $line =~ /^END Game_Summary$/ ) {
		out_csa $ref_status, $sckt_csa, $fh_log, "AGREE";
		next;
	    }
	    
	    if ( $line =~/^REJECT/ ) {
		@$ref_game_summary = ();
		next;
	    }
	    
	    if ( $line =~/^START/ ) {
		$game_agreed = 1;
		next;
	    }
	    
	    push @$ref_game_summary, $line;
	}
    }

    # parse massages of the game summary from the CSA Shogi server
    my $color_last    = '-';

    $$ref_status{pid}        = 0;
    $$ref_status{name1}      = "";
    $$ref_status{name2}      = "";
    $$ref_status{color}      = '+';
    $$ref_status{phase}      = phase_puzzling;
    $$ref_status{sec_mytime} = 0;
    $$ref_status{sec_optime} = 0;
    $$ref_status{buf_record} = [];
    $$ref_status{buf_resume} = [];
    out_clients $ref_status, $ref_sckt_clients, $fh_log, "new";


    foreach my $line ( @$ref_game_summary ) {

	if ( $line =~ /^Your_Turn\:([+-])\s*$/ ) { $$ref_status{color} = $1; }
	elsif ( $line =~ /^([+-])(\d\d\d\d\w\w),T(\d+)/ ) {
	    push @{$$ref_status{buf_record}}, "$1$2,T$3";
	    $$ref_status{pid}        += 1;
	    $color_last               = $1;
	    out_clients( $ref_status, $ref_sckt_clients, $fh_log,
			 "move $2 $$ref_status{pid}" );
	}
	elsif ( $line =~ /^Name\+\:(\w+)/ ) { $$ref_status{name1} = $1; }
	elsif ( $line =~ /^Name\-\:(\w+)/ ) { $$ref_status{name2} = $1; }
    }

    if ( not $$ref_status{color} eq $color_last ) {
	
	$$ref_status{phase} = phase_thinking;
    }

    if ( $$ref_status{color} eq '+') {

	$$ref_status{sec_mytime} = $$ref_status{sec_spent_b};
	$$ref_status{sec_optime} = $$ref_status{sec_spent_w};

    } else {

	$$ref_status{sec_mytime} = $$ref_status{sec_spent_w};
	$$ref_status{sec_optime} = $$ref_status{sec_spent_b};
    }

    return $sckt_csa;
}


sub out_csa ($$$$) {
    my ( $ref_status, $sckt_csa, $fh_log, $line ) = @_;

    $$ref_status{time_last_send} = $$ref_status{time};
    print $sckt_csa "$line\n";   # \n is assumed to be represented by 0x0a.
    out_log $fh_log, "csa< $line";
}


sub out_clients ($$$$) {
    my ( $ref_status, $ref_fh, $fh_log, $line ) = @_;

    foreach my $fh ( @$ref_fh ) { print $fh "$line\n"; }
    out_log $fh_log, "all< $line";
    push @{$$ref_status{buf_resume}}, $line;
}



sub select_sckts ($$$$) {
    my ( $sckt_listen, $sckt_csa, $ref_status, $ref_sckt_clients ) = @_;
    my ( @sckt_ready );

    # check if any buffers had already read one line
    @sckt_ready	= grep( { index( ${$$ref_status{$_}}{buf}, "\n" ) != -1 }
			@$ref_sckt_clients );

    if ( index( $$ref_status{buf_csa}, "\n" ) != -1 ) {
	push @sckt_ready, $sckt_csa;
    }

    if ( @sckt_ready ) { return @sckt_ready; }
    
    # get messages from all sockets
    my $selector
	= new IO::Select $sckt_listen, $sckt_csa, @$ref_sckt_clients;

    foreach my $sckt ( $selector->can_read( $$ref_status{timeout} ) ) {

	if ( $sckt == $sckt_listen ) {

	    # connect() from a client
	    my $sckt = $sckt_listen->accept or die "accept() failed: $!\n";
	      
	    push @$ref_sckt_clients, $sckt;
	    ${$$ref_status{$sckt}}{buf}    = "";
	    ${$$ref_status{$sckt}}{id}     = "no-name";
	    ${$$ref_status{$sckt}}{factor} = 1.0;
	    foreach my $line ( @{$$ref_status{buf_resume}} ) {
		out_client $sckt, $line;
	    }
	    next;
	}

	my $input;
	if ( $sckt_csa and $sckt == $sckt_csa ) {

	    # message arrived from CSA Shogi server
	    $sckt->recv( $input, 65536 );
	    $input or die "connection to CSA Shogi server is down: $!\n";

	    $$ref_status{buf_csa} .= $input;
	    next;
	}

	# message arrived from a client
	my $ref = $$ref_status{$sckt};

	$sckt->recv( $input, 65536 );
	unless ( $input ) {
	    # One client is down
	    my $nclient = @$ref_sckt_clients - 1;

	    warn "\nWARNING: connection to $$ref{id} is down. "
		. "$nclient clients left.\n\n";

	    @$ref_sckt_clients = grep { $_ != $sckt; } @$ref_sckt_clients;

	    delete $$ref_status{$sckt};
	    $selector->remove( $sckt );
	    $sckt->close;
	    next;
	}

	$$ref{buf} .= $input;
    }

    # check if any buffers received one line now
    push( @sckt_ready,
	  grep( { index( ${$$ref_status{$_}}{buf}, "\n" ) != -1 }
		@$ref_sckt_clients ) );

    if ( index( $$ref_status{buf_csa}, "\n" ) != -1 ) {
	push @sckt_ready, $sckt_csa;
    }

    return @sckt_ready;
}


sub parse_clogin ($$) {
    my ( $ref, $line ) = @_;

    ( $$ref{id}, $$ref{factor} ) = split " ", $line;
    $$ref{login}                 = 1;
    $$ref{have_stable}           = 1 if $line =~ /stable/;
    $$ref{have_final}            = 1 if $line =~ /final/;
    $$ref{have_confident}        = 1 if $line =~ /confident/;
		
    print "  $line is accepted\n";
}


sub get_line ($) {
    my ( $ref_buf ) = @_;
    my ( $pos, $line );

    $pos = index $$ref_buf, "\n";
    if ( $pos == -1 ) { die "$!"; }

    $line     = substr $$ref_buf, 0, 1+$pos;
    $$ref_buf = substr $$ref_buf, 1+$pos;
    $line     =~ s/\r?\n$//;

    return $line;
}
