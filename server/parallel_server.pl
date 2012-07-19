#!/usr/bin/perl -w

# --- hash keys ---
# client: sckt, id, iaddr, port, in_buf, stable, final, played_move
#         best_move, nodes, value, pid
#
# score:  move, value, nodes, final, stable
# -----------------

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

# constants
sub timeout    ()    { 180.0 } # keepalive time
sub send_freq  ()    {   0.5 }
sub maxeval_score () { 30_000 }

# subroutines
sub wait_messages($$$$$);
sub out_clients ($$);
sub parse_smsg($$$);
sub parse_cmsg($$$);
sub get_line ($);
sub next_log ($);
sub out_log ($$);


my $global_pid = 0;

{
    # defaults of command-line options
    my ( %status ) = ( client_port      => 4084,
		       client_num       => 3,
		       server_host      => 'localhost',
		       server_port      => 4082,
		       server_id        => 'Parallel1',
		       server_buf       => q{},
		       server_send_time => time(),
		       server_send_mmsg => q{},
		       split_nodes      => 100_000,
		       final            => 0,
		       sent_final       => 0,
		       stable           => 1,
		       server_pid       => 0,
		       fh_log           => undef );


    # parse command-line options
    GetOptions( \%status,
		'client_port=i',
		'client_num=i',
		'server_host=s',
		'server_port=i',
		'server_id=s',
		'split_nodes=i' ) or die "$!";

    # creates a listening socket for my clients.
    my $listening_sckt
	= new IO::Socket::INET( LocalPort => $status{client_port},
				Listen    => SOMAXCONN,
				Proto     => 'tcp',
				ReuseAddr => 1 )
	or die "Can't create a listening socket: $!\n";

    print "wait for $status{client_num} clients:\n";

    my ( %client );
    my ( $server_sckt, $count );
    while ( 1 ) {

	foreach my $ready_key ( wait_messages( $listening_sckt,
					       \%client, undef, undef,
					       10.0 ) ) {
	    
	    my $client_ref = $client{$ready_key};

	    # get one line from buffer
	    my $line = get_line \$client_ref->{in_buf};
	    if ( 'unknown' eq $client_ref->{id} ) {
		my ( $final, $stable ) = ( 0, 0 );
		print( "LOGIN: $line"
		       . " ($client_ref->{iaddr}:$client_ref->{port})\n" );
		$client_ref->{id} = ( split ' ', $line )[0];
		if ( $line =~ /stable/ ) { $stable = 1; }
		if ( $line =~ /final/ )  { $final  = 1; }
		$status{stable} &= $stable;
		$status{final}  |= $final;
	    }
	}
	
	# connect to server when all clients finished login
	if ( $status{client_num}
	     == grep( { defined $_->{id} and not $_->{id} eq 'unknown' }
		      values %client ) ) {

	    $server_sckt
		= new IO::Socket::INET( PeerAddr => $status{server_host},
					PeerPort => $status{server_port},
					Proto    => 'tcp' );
	    if ( $server_sckt ) {
		my $final  = q{};
		my $stable = q{};
		if ( $status{final} )  { $final  = 'final'; }
		if ( $status{stable} ) { $stable = 'stable'; }
		print $server_sckt "$status{server_id} 1.0 $final $stable\n";
		last;
	    }
	    
	    if ( $count ) { print "."; }
	    else          { print "connect() to server faild, try again "; }
	    $count += 1;
	}
    }


    # main loop
    my $server_buf       = q{};
    my $server_send_time = time;
    while ( 1 ) {
	
	my ( @ready_keys) = wait_messages( $listening_sckt, \%client,
					   $server_sckt, \$server_buf,
					   timeout );
	my $time = time();

	foreach my $ready_key ( @ready_keys ) {

	    if ( $ready_key eq 'server socket' ) {
		my $line = get_line \$server_buf;
		if ( not parse_smsg \%status, \%client, $line ) {
		    die "MESSAGE FROM SERVER: $line";
		}
	    }
	    else {
		my $line = get_line \$client{$ready_key}->{in_buf};
		if ( not parse_cmsg \%status, $client{$ready_key}, $line ) {
		    die "MESSAGE FROM CLIENT: $line\n";
		}
	    }
	}


	# no more messages to server after 'final'
	if ( $status{sent_final} ) {

	    # send a null line for keepalive
	    if ( not @ready_keys ) {
		out_log \%status, "Server< \n";
		print $server_sckt "\n";
	    }
	    next;
	}


	# organize client opinion into %score
	my $have_unexpanded    = 0;
	my ( %score )          = ();
	my ( @expanded_moves ) = ();
	foreach my $client_ref ( values %client ) {
	    
	    my ( $move, $value, $expanded );

	    if ( defined $client_ref->{played_move} ) {
		$move          =  $client_ref->{played_move};
		$value         = -$client_ref->{value};
		$expanded      = 1;
		push @expanded_moves, $move;
	    }
	    elsif ( defined $client_ref->{best_move} ) {
		$move     = $client_ref->{best_move};
		$value    = $client_ref->{value};
		$expanded = 0;
		$have_unexpanded = 1;
	    }
	    else { next; }

	    my $client_pid = $client_ref->{pid};
	    if ( exists $score{$client_pid} ) {

		if ( $score{$client_pid}->{final} ) { next; }
		elsif ( not $client_ref->{final}
			and ( $client_ref->{nodes}
			      <= $score{$client_pid}->{nodes} ) ) { next; }

	    }

	    $score{$client_pid} = { move       => $move,
				    value      => $value,
				    client_ref => $client_ref,
				    nodes      => $client_ref->{nodes},
				    final      => $client_ref->{final},
				    stable     => $client_ref->{stable},
				    expanded   => $expanded };
	}


	# send a message to the server if necessary
	if ( keys %score ) {
	    my ( $best_ref, $best_value, $final, $stable_str, $final_str );
	    my $node_sum  = 0;
	    my $moves_num = 0;
	    my $stable    = 1;
	    foreach my $score_ref ( values %score ) {
		$node_sum += $score_ref->{nodes};
		$stable   &= $score_ref->{stable};

		# count legal moves at root
		if ( $score_ref->{value} < - maxeval_score()
		     and $score_ref->{final} )   { $moves_num += 0; }
		elsif ( $score_ref->{expanded} ) { $moves_num += 1; }
		else                             { $moves_num += 100; }

		# find the best child
		if ( not defined $best_value
		     or $best_value < $score_ref->{value} ) {
		    $best_ref   = $score_ref;
		    $best_value = $best_ref->{value};
		}
	    }
	    
	    if ( $best_ref->{final}
		 or ( $moves_num < 2 and $have_unexpanded ) ) { $final = 1; }
	    else                                              { $final = 0; }

	    if ( $status{stable} and $stable ) { $stable_str = ' stable'; }
	    else                               { $stable_str = q{}; }

	    if ( $status{final} and $final )   { $final_str = ' final'; }
	    else                               { $final_str = q{}; }

	    my $mmsg =( "pid=$status{server_pid} move=$best_ref->{move} "
			. "v=${best_value}e$stable_str$final_str" );
	    
	    if ( defined $status{server_send_mmsg}
		 and $status{server_send_mmsg} eq $mmsg ) {
		
		if ( $status{server_send_time} + send_freq < $time ) {
		    my $line = "pid=$status{server_pid} n=$node_sum";
		    out_log \%status, "Server< $line\n";
		    print $server_sckt "$line\n";
		    $status{server_send_time} = $time;
		    if ( $final_str ) { $status{sent_final} = 1; }
		}
	    }
	    else {
		my $line = "$mmsg n=$node_sum";
		out_log \%status, "Server< $line\n";
		print $server_sckt "$line\n";
		$status{server_send_time} = $time;
		$status{server_send_mmsg} = $mmsg;
		if ( $final_str ) { $status{sent_final} = 1; }
	    }
	}

	# split root if possible
	my ( $score_ref ) = ( grep { not $_->{expanded} } values %score );
	my $dbg_expand     = 0;
	my $dbg_not_expand = 0;
	if ( defined $score_ref
	     and not $score_ref->{final}
	     and @expanded_moves + 1 < keys %client
	     and $status{split_nodes} < $score_ref->{nodes} ) {
	    
	    my $move  = $score_ref->{move};
	    my $first = 1;
	    foreach my $client_ref ( values %client ) {

		if ( defined $client_ref->{played_move} ) { next; }

		if ( $first ) {
		    my $pid  = $global_pid + 1;
		    my $line = "move $move $pid";
		    out_log \%status, "$client_ref->{id}< $line\n";
		    print { $client_ref->{sckt} } "$line\n";
		    $client_ref->{played_move} = $move;
		    $client_ref->{best_move}   = undef;
		    $client_ref->{final}       = 0;
		    $client_ref->{stable}      = 0;
		    $client_ref->{nodes}       = 0;
		    $client_ref->{value}       = -$score_ref->{value};
		    $client_ref->{pid}         =  $pid;
		    $first                     = 0;
		    $dbg_expand               += 1;
		}
		else {
		    my $pid   = $global_pid + 2;
		    my $moves = join ' ', ( @expanded_moves, $move );
		    my $line  = "ignore $pid $moves";
		    out_log \%status, "$client_ref->{id}< $line\n";
		    print { $client_ref->{sckt} } "$line\n";
		    $client_ref->{best_move} = undef;
		    $client_ref->{final}     = 0;
		    $client_ref->{stable}    = 0;
		    $client_ref->{nodes}     = 0;
		    $client_ref->{value}     = 0;
		    $client_ref->{pid}       =  $pid;
		    $dbg_not_expand         += 1;
		}
	    }
	    if ( $dbg_expand != 1 or $dbg_not_expand < 1 ) { die; }
	    $global_pid += 2;
	}
    }
}


sub parse_smsg($$$) {
    my ( $status_ref, $client_ref, $line ) = @_;
		
    out_log $status_ref, "\nServer> $line\n";

    if ( $line =~ /^\s*$/ ) { return 1; }
    elsif ( $line eq 'idle' ) {
	foreach my $ref ( values %$client_ref ) {
	    $status_ref->{sent_final} = 1;
	    out_log $status_ref, "$ref->{id}< idle\n";
	    print { $ref->{sckt} } "idle\n";
	}
    }
    elsif ( $line eq 'new' ) {
	next_log $status_ref;
	$status_ref->{sent_final}       = 0;
	$status_ref->{server_send_mmsg} = q{};
	$status_ref->{server_pid}       = 0;
	$global_pid                     = 0;
	foreach my $ref ( values %$client_ref ) {
	    out_log $status_ref, "$ref->{id}< new\n";
	    print { $ref->{sckt} } "new\n";
	    $ref->{played_move} = undef;
	    $ref->{best_move}   = undef;
	    $ref->{final}       = 0;
	    $ref->{stable}      = 0;
	    $ref->{nodes}       = 0;
	    $ref->{value}       = 0;
	    $ref->{pid}         = 0;
	}
    }
    elsif ( $line =~ /move (\d\d\d\d[A-Z][A-Z]) (\d+)$/ ) {
	$status_ref->{sent_final}  = 0;
	$status_ref->{server_pid}  = $2;
	$global_pid               += 1;

	my $pid = $global_pid;
	foreach my $ref ( values %$client_ref ) {
	    if ( defined $ref->{played_move} and $ref->{played_move} eq $1 ) {
		$pid = $ref->{pid};
		last;
	    }
	}

	foreach my $ref ( values %$client_ref ) {

	    if ( defined $ref->{played_move}
		 and $ref->{played_move} eq $1 )  {
		out_log $status_ref, "$ref->{id} has the same position.\n";
		$ref->{played_move} = undef;
		$ref->{value}       = -$ref->{value};
		$ref->{pid}         = $pid;
	    }
	    else {
		if ( defined $ref->{played_move} ) {
		    $line= "alter $1 $global_pid";
		}
		else {
		    $line= "move $1 $global_pid";
		}
		out_log $status_ref, "$ref->{id}< $line\n";
		print { $ref->{sckt} } "$line\n";
		$ref->{played_move} = undef;
		$ref->{best_move}   = undef;
		$ref->{final}       = 0;
		$ref->{stable}      = 0;
		$ref->{nodes}       = 0;
		$ref->{value}       = 0;
		$ref->{pid}         = $pid;
	    }
	}
    }
    elsif ( $line =~ /alter (\d\d\d\d[A-Z][A-Z]) (\d+)$/ ) {
	$status_ref->{sent_final}  = 0;
	$status_ref->{server_pid}  = $2;
	$global_pid               += 1;
	foreach my $ref ( values %$client_ref ) {

	    if ( defined $ref->{played_move} ) {
		$line = "retract 2 $1 $global_pid";
	    }
	    else { $line = "alter $1 $global_pid"; }

	    out_log $status_ref, "$ref->{id}< $line\n";
	    print { $ref->{sckt} } "$line\n";
	    $ref->{played_move} = undef;
	    $ref->{best_move}   = undef;
	    $ref->{final}       = 0;
	    $ref->{stable}      = 0;
	    $ref->{nodes}       = 0;
	    $ref->{value}       = 0;
	    $ref->{pid}         = $global_pid;
	}
    }
    else { return 0; }

    return 1;
}


sub parse_cmsg($$$) {
    my ( $status_ref, $client_ref, $line ) = @_;
    my ( $client_pid, $client_move, $client_nodes, $client_value );
    
    out_log $status_ref, "$client_ref->{id}> $line\n";

    if ( $line =~ /^$/ ) {
	print { $client_ref->{sckt} } "\n";
	return 1;
    }

    if( $line =~ /pid=(\d+)/ ) { $client_pid = $1; }

    if ( not defined $client_pid )           { return 0; }
    if ( $client_pid != $client_ref->{pid} ) { return 1; }

    my $client_final  = 0;
    my $client_stable = 0;
    if    ( $line =~ /move=(\d\d\d\d[A-Z][A-Z])/ ) { $client_move = $1 }
    elsif ( $line =~ /move=%TORYO/ )               { $client_move = '%TORYO'; }

    if ( $line =~ /n=(\d+)/ )                   { $client_nodes  = $1 }
    if ( $line =~ /v=([+-]?\d+)/ )              { $client_value  = $1 }
    if ( $line =~ /final/ )                     { $client_final  = 1 }
    if ( $line =~ /stable/ )                    { $client_stable = 1 }
    

    if ( defined $client_move
	 and defined $client_nodes
	 and defined $client_value ) {
	$client_ref->{best_move} = $client_move;
	$client_ref->{nodes}     = $client_nodes;
	$client_ref->{value}     = $client_value;
	$client_ref->{final}     = $client_final;
	$client_ref->{stable}    = $client_stable;
    }
    elsif ( $client_final ) { $client_ref->{final} = $client_final; }
    elsif ( $client_nodes ) { $client_ref->{nodes} = $client_nodes; }
    else { return 0; }

    return 1;
}
    

sub wait_messages($$$$$) {

    my ( $listening_sckt, $client_ref, $server_sckt, $server_buf_ref,
	$timeout ) = @_;
    my ( @ready_keys );
    
    # get all handles to wait for messages
    my ( @handles ) = ( $listening_sckt,
			map { $client_ref->{$_}->{sckt} } keys %$client_ref );
    if ( defined $server_sckt ) { push @handles, $server_sckt; }

    my $selector = new IO::Select( @handles );
    
    while (1) {
	@ready_keys
	    = grep( { index( $client_ref->{$_}->{in_buf}, "\n" ) != -1 }
		    keys %$client_ref );

	if ( defined $server_buf_ref
	     and index( $$server_buf_ref, "\n" ) != -1 ) {
	    push @ready_keys, 'server socket';
	}
	if ( @ready_keys ) { last; }

	my ( @sckts ) = $selector->can_read( $timeout );
	if ( not @sckts ) { return; }

	foreach my $in ( @sckts ) {
	    
	    if ( $in eq $listening_sckt ) {
		
		# accept the connection for a client
		my $accepted_sckt = $in->accept or die "accept failed: $!\n";
		my ( $port, $iaddr )
		    = unpack_sockaddr_in $accepted_sckt->peername;
		my $str_iaddr = inet_ntoa $iaddr;
		
		$selector->add( $accepted_sckt );
		$client_ref->{ $accepted_sckt }	= {
		    id          => 'unknown',
		    iaddr       => $str_iaddr,
		    port        => $port,
		    in_buf      => q{},
		    sckt        => $accepted_sckt,
		    stable      => 0,
		    final       => 0,
		    played_move => undef,
		    best_move   => undef,
		    nodes       => 0,
		    value       => 0,
		    pid         => 0 };
		
		print $accepted_sckt "idle\n";
		next;
	    }
	    
	    # message arrived from a server
	    if ( defined $server_sckt and $in eq $server_sckt ) {
		
		my $in_buf;
		$in->recv( $in_buf, 65536 );
		if ( not $in_buf ) { die 'The server is down.'; }
		$$server_buf_ref .= $in_buf;
		$$server_buf_ref  =~ s/\015?\012/\n/g;
		next;
	    }

	    # message arrived from a client
	    my $in_buf;
	    $in->recv( $in_buf, 65536 );
	    if ( not $in_buf ) {
		die( "connection from $client_ref->{$in}->{iaddr}"
		     . " $client_ref->{$in}->{port}"
		     . " ($client_ref->{$in}->{id}) is down.\n" );
	    }
	    $client_ref->{$in}->{in_buf} .= $in_buf;
	    $client_ref->{$in}->{in_buf}  =~ s/\015?\012/\n/g;
	}
    }
    
    return @ready_keys;
}


sub get_line ($) {
    my ( $line_ref ) = @_;

    my $pos = index $$line_ref, "\n";
    if ( $pos == -1 ) { die 'Internal error' }
	    
    my $line   = substr $$line_ref, 0, 1+$pos;
    $$line_ref = substr $$line_ref, 1+$pos;
    chomp $line;

    return $line;
}


sub out_clients ($$) {
    my ( $client_ref, $line ) = @_;

    print "ALL< $line\n";
    foreach my $client_key ( keys %$client_ref ) {
	print { $client_ref->{$client_key}->{sckt} } "$line\n";
    }
}


sub next_log ($) {
    my ( $status_ref ) = @_;

    if ( defined $status_ref->{fh_log} ) {
	$status_ref->{fh_log}->close or die "$!";
    }

    my ( $sec, $min, $hour, $mday, $mon, $year ) = localtime( time );
    $year -= 100;
    $mon  += 1;
    my $dirname = sprintf "parallel_%02s%02s", $year, $mon;
    if ( not -d $dirname ) { mkdir $dirname or die "$!"; }

    my $basename = sprintf "%02s%02s%02s%02s", $mday, $hour, $min, $sec;
    my $filename = File::Spec->catfile( $dirname, $basename );

    open $status_ref->{fh_log}, ">${filename}.log" or die "$!";
    $status_ref->{fh_log}->autoflush(1);
}


sub out_log ($$) {
    my ( $status_ref, $msg ) = @_;

    if ( defined $status_ref->{fh_log} ) {
	print { $status_ref->{fh_log} } $msg;
    }
    else { print $msg; }
}
